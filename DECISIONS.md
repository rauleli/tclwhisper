## D-001 — Runtime model ownership

Status: proposed

### Question

Should tclwhisper expose one process-wide/interpreter-wide Whisper
instance through the `whisper` ensemble, or support multiple model
instances?

### Candidate

Singleton API:

    whisper init model
    whisper transcribe ...
    whisper free

### Rationale

Matches the intended simple tclllama-style usage and avoids designing
multi-instance lifecycle semantics without a demonstrated requirement.

### Evidence

Pending characterization of tclllama and whisper.cpp.

### Decision

Pending conclave review.

## D-002 — Bootstrap experimental de tclwhisper antes de congelar su API

**Status:** approved

### Question

¿Cómo debe comenzar el desarrollo de `tclwhisper` sin convertir hipótesis de diseño en una API permanente antes de observar el comportamiento real de `whisper.cpp`?

En particular, todavía existen decisiones abiertas sobre:

* singleton frente a handles;
* representación exacta del PCM recibido desde Tcl;
* soporte de `f32` únicamente o conversión adicional desde `s16`;
* necesidad real de una opción de sample rate;
* estrategia greedy frente a beam search;
* estructura de resultados detallados;
* política de logging;
* opciones GPU;
* parámetros de inferencia que deben exponerse o encapsularse.

### Evidence

El análisis conjunto de Claude, DeepSeek y ChatGPT mostró que la propuesta inicial de API contenía una mezcla de tres clases distintas de elementos:

1. requisitos impuestos realmente por `whisper.cpp`;
2. decisiones propias del binding Tcl;
3. decisiones heredadas por analogía con versiones anteriores o documentación desactualizada de `tclllama`.

La inspección del código actual de `tclllama` mostró además que su implementación vigente es handle-based, aunque parte de su documentación todavía describe una interfaz anterior. Por tanto, singleton no puede adoptarse simplemente bajo el argumento de mantener simetría con `tclllama`.

También se estableció que `whisper.cpp` proporciona directamente una API nativa suficiente para realizar un primer probe sin cargar modelos ni procesar audio mediante:

```c
whisper_version()
```

El upstream seleccionado para iniciar la caracterización es:

```text
whisper.cpp v1.9.4
```

La dependencia se mantiene fuera del árbol de `tclwhisper`. El source, build e instalación local de `whisper.cpp` pueden existir como directorios hermanos, evitando vendorizar upstream dentro del binding.

### Decision

El desarrollo de `tclwhisper` comienza mediante slices pequeños, verificables y acumulativos.

El **Slice 0** queda limitado a:

```tcl
package require tclwhisper
whisper::version
```

`whisper::version` debe devolver el valor real obtenido de:

```c
whisper_version()
```

y no una cadena hardcodeada.

Durante este slice se debe caracterizar también:

* cómo se descubren los headers de `whisper.cpp`;
* cómo se enlaza `libwhisper`;
* qué dependencias compartidas intervienen;
* si `pkg-config` y `whisper.pc` proporcionan una interfaz suficiente;
* comportamiento de runtime y rpath;
* tag y commit exactos del upstream;
* configuración de build utilizada;
* versión reportada realmente por la biblioteca.

No se implementarán todavía:

* carga de modelos;
* lifecycle de contextos;
* singleton ni handles;
* transcripción;
* PCM;
* conversiones de formatos;
* sample rate como opción Tcl;
* GPU;
* Flash Attention;
* threading;
* logging;
* segmentos;
* timestamps;
* greedy o beam search;
* resultados detallados.

Finalizado el Slice 0 se detiene el trabajo y se presenta evidencia al cónclave antes de diseñar el siguiente slice.

### Consequences

`DECISIONS.md` no congela todavía la API pública de `tclwhisper`.

Las decisiones futuras se tomarán sobre comportamiento observado y pruebas ejecutables, no solamente por simetría con otros bindings o por conveniencia aparente.

`whisper.cpp` permanece como dependencia externa y separada del repositorio de `tclwhisper`.

El primer artefacto funcional de `tclwhisper` será deliberadamente pequeño. Su propósito es validar la cadena:

```text
Tcl
 ↓
tclwhisper
 ↓
libwhisper
 ↓
whisper.cpp
```

antes de introducir estado, modelos o audio.

Esta decisión establece además el patrón metodológico para los siguientes slices:

```text
inspeccionar
→ caracterizar
→ implementar el mínimo coherente
→ probar
→ reportar
→ revisar
→ decidir el siguiente slice
```

## D-003 — Lifecycle y ownership de contextos

**Status:** approved

### Question

¿Cómo debe representar y administrar `tclwhisper` los contextos nativos de `whisper.cpp`?

### Evidence

`whisper.cpp` permite crear contextos independientes.

La implementación actual de `tclllama` también es handle-based, aunque su mecanismo concreto de destrucción no debe copiarse automáticamente.

Tcl permite asociar estado nativo a un comando mediante `ClientData` y registrar una rutina de destrucción del comando. Esto permite que la liberación de recursos ocurra tanto en un `free` explícito como cuando el comando desaparece por otros caminos de Tcl.

### Decision

`tclwhisper` será **handle-based**.

Cada:

```tcl
set w [whisper::init $model]
```

crea un `whisper_context` independiente y devuelve un handle Tcl que representa y posee ese contexto.

El binding no impone singleton.

El contexto nativo pertenece al handle. Toda ruta de destrucción del handle debe converger en **una única rutina de cleanup** que libere exactamente una vez el `whisper_context` y cualquier estado asociado.

Esto incluye al menos:

* `whisper::free $w`;
* eliminación explícita del comando-handle;
* `rename $w {}`;
* destrucción del intérprete que contiene el handle.

`whisper::free` no implementará una segunda ruta independiente de liberación. Debe provocar la destrucción del comando-handle y dejar que la rutina común de cleanup libere los recursos.

Un fallo durante `whisper::init` no debe publicar un handle parcial ni dejar un contexto nativo vivo.

La generación y administración de nombres de handles debe permanecer local al intérprete. El mecanismo concreto se decidirá durante la implementación y no forma parte de la API pública.

### Consequences

Iik’ puede mantener normalmente un solo contexto STT por proceso sin que esa política se convierta en una restricción del binding.

También pueden existir múltiples contextos independientes cuando un consumidor legítimamente los necesite.

Eliminar un handle equivale a destruir el recurso nativo asociado.

Después de su destrucción, ese nombre deja de identificar un contexto válido y las operaciones posteriores deben fallar como referencias a un handle inexistente.

El mecanismo concreto de implementación se elegirá según las garantías de Tcl y `whisper.cpp`, no por copia mecánica de `tclllama`.

## D-004 — Preparación de la ruta del modelo

**Status:** approved

### Question

¿Debe `tclwhisper` expandir o normalizar automáticamente la ruta recibida por
`whisper::init` antes de entregarla a whisper.cpp?

### Decision

`tclwhisper` no expande ni normaliza la ruta del modelo antes de entregarla a
upstream. En particular, no expande `~` ni canonicaliza componentes relativos
o enlaces simbólicos. Si el llamador necesita una ruta normalizada, debe
prepararla previamente mediante las operaciones de filesystem de Tcl, por
ejemplo `file normalize`. El binding entrega a upstream la representación
recibida de Tcl.

### Consequences

- El uso efectivo de rutas relativas o enlaces simbólicos queda sujeto al
  comportamiento normal de upstream y del sistema de archivos; el binding no
  los canonicaliza previamente.
- No se añade una API de filesystem.
- Esta decisión documenta el contrato de `whisper::init` y no introduce
  funcionalidad perteneciente a Slice 2.

## D-005 — Contrato PCM inicial de transcripción

**Status:** approved

### Decision

La primera ruta de transcripción de `tclwhisper` acepta PCM mono, 16000 Hz,
float32 little-endian, con muestras nominalmente normalizadas en el rango
`[-1.0,+1.0]`.

El binding no contiene adquisición de audio, decodificación de archivos,
resampling, mezcla de canales ni normalización. El productor del PCM es
responsable de cumplir el contrato.

Una entrada PCM de longitud cero representa ausencia de audio y devuelve una
cadena vacía sin invocar `whisper_full()`.

Este es el contrato inicial de Slice 2 y no implica que formatos adicionales
no puedan incorporarse posteriormente.

## D-006 — Selección de idioma por transcripción

**Status:** approved

### Decision

`whisper::transcribe` permite seleccionar el idioma para una llamada individual
mediante `-language`. Sin la opción se conserva el comportamiento default
utilizado desde Slice 2.

La sintaxis pública es:

```text
whisper::transcribe <handle> <pcm> ?-language <language|auto>?
```

Un identificador explícito validado mediante `whisper_lang_id()` selecciona el
idioma únicamente para la llamada actual. `-language auto` utiliza la semántica
nativa de `params.language = "auto"`: autodetecta el idioma y continúa con la
transcripción, sin activar la operación separada `params.detect_language`.

La cadena vacía no es un alias público de `auto`; la autodetección debe
solicitarse explícitamente mediante `-language auto`.

La selección no se almacena en el handle ni afecta llamadas posteriores.
`tclwhisper` no mantiene una tabla propia de idiomas y delega a upstream la
aceptación de códigos y nombres, excepto por las restricciones deliberadas de
esta API.

### Observed upstream acceptance

- `"es"` es aceptado y resuelve al identificador 3 (`es`).
- `"spanish"` es aceptado y resuelve al identificador 3 (`es`).
- `"ES"` es rechazado.
- `"es "` —con un espacio final— es rechazado.
- `""` es rechazado deliberadamente por `tclwhisper`, aunque upstream lo
  interpreta como solicitud de autodetección.
- `"auto"` es el valor especial público para autodetección seguida de
  transcripción.
- Upstream compara exactamente: el binding no normaliza mayúsculas, espacios,
  nombres ni aliases.


## D-007 — Threads por transcripción (Slice 4)

**Status:** approved

`whisper::transcribe handle pcm ?-language language|auto? ?-n_threads integer?`
acepta ambas opciones en cualquier orden y como máximo una vez cada una.
`-n_threads` es por llamada: no se almacena en el handle ni persiste en llamadas
posteriores. Al omitirse conserva exactamente el valor de
`whisper_full_default_params(WHISPER_SAMPLING_GREEDY)`, sin copiar su default
al binding. Al especificarse modifica únicamente `params.n_threads`.

Se exige un entero positivo representable por el `int` de upstream; no se
impone un máximo arbitrario propio. La validación precede a la conversión,
copia y shortcut de PCM vacío y a la inferencia. La aceptación del rango no
promete recursos suficientes para ejecutar valores extremos.

No cambia ningún otro control de reconocimiento ni el lifecycle. La evidencia
RTX y las limitaciones de la matriz constan en `CHARACTERIZATION.md`.

## D-008 — Soporte `s16le` diferido hasta caracterizar el pipeline real de audio

**Status:** approved

### Question

¿Debe `tclwhisper` ampliar `whisper::transcribe` para aceptar PCM `s16le` y
convertirlo internamente a `float32`, o debe conservar por ahora su contrato
exclusivo de PCM `f32le`?

### Evidence

D-005 estableció como contrato inicial que `tclwhisper` recibe PCM:

```text
mono
16000 Hz
IEEE-754 float32 little-endian
```

y que el binding no realiza adquisición de audio, resampling, mezcla de
canales, decodificación de archivos ni normalización.

`whisper.cpp` consume muestras `const float *` en `whisper_full()` y no expone
una ruta nativa de inferencia que acepte directamente PCM `int16`.

La conversión candidata para `s16le` está técnicamente bien entendida:

```text
float_sample = int16_sample / 32768.0f
```

con:

```text
-32768 → -1.0
0      → 0.0
32767  → 0.999969482421875
```

Por tanto, no existe una incertidumbre técnica importante sobre cómo podría
implementarse esa conversión si se aprobara posteriormente.

Sin embargo, todavía no existe evidencia del pipeline final de adquisición y
normalización de Iik’ que demuestre que `tclwhisper` deba poseerla.

En particular, todavía debe caracterizarse:

1. qué representación PCM entrega realmente el hardware o la capa de
   adquisición;
2. qué componente recibe esa representación;
3. qué transformaciones realiza dicho componente;
4. qué representación PCM entrega finalmente a `tclwhisper`;
5. si existe una conversión redundante que aceptar `s16le` directamente en
   `tclwhisper` permitiría eliminar.

El argumento de rendimiento frente a convertir muestra por muestra en Tcl
puro no es suficiente por sí solo para decidir la frontera, porque la
arquitectura no obliga a que esa conversión viva en Tcl. Puede existir una
capa nativa reutilizable de audio antes del STT.

### Decision

`tclwhisper` **no incorporará soporte `s16le` por ahora**.

El contrato público de entrada permanece:

```text
mono
16000 Hz
IEEE-754 float32 little-endian
```

No se añadirá en este momento:

```tcl
-format s16le
```

ni ninguna otra opción de formato PCM.

Esta decisión continúa la frontera establecida por D-005: `tclwhisper` recibe
audio ya preparado para la representación nativa que consume `whisper.cpp` y
no asume responsabilidades generales de normalización de audio.

Esta decisión no rechaza técnicamente `s16le` ni declara que nunca deba
incorporarse.

La propuesta queda diferida hasta caracterizar el pipeline real de
adquisición y normalización de Iik’.

### Reconsideration criteria

Esta decisión deberá revisarse cuando el pipeline de audio de Iik’ esté
suficientemente caracterizado para responder explícitamente:

- qué formato entrega el hardware o la captura;
- qué capa recibe ese formato;
- si existe una capa nativa de conversión o normalización;
- qué formato entrega esa capa a `tclwhisper`;
- si aceptar `s16le` directamente en `tclwhisper` eliminaría una conversión
  real o únicamente desplazaría/duplicaría una responsabilidad.

Si el pipeline final no incluye una capa nativa de normalización y entrega
`s16le` directamente hacia el consumidor STT, entonces la conversión
`s16le → float32` deberá existir en algún componente. En ese escenario,
`tclwhisper` podrá reconsiderarse como candidato natural para alojarla.

### Preserved implementation candidate

Si evidencia futura justifica soporte `s16le`, la candidata actualmente mejor
entendida sería:

```tcl
whisper::transcribe $handle $pcm \
    ?-format f32le|s16le? \
    ?-language language|auto? \
    ?-n_threads integer?
```

con:

```text
default: f32le
s16le conversion: int16 / 32768.0f
```

y sin introducir:

- resampling;
- channel mixing;
- normalización de volumen;
- file decoding;
- WAV/RIFF parsing;
- adquisición ALSA;
- VAD;
- streaming.

Esta sección preserva únicamente un diseño candidato. No aprueba esa API ni
forma parte del contrato público actual.

### Consequences

- `tclwhisper` conserva una frontera pequeña y estable.
- Se mantiene la continuidad arquitectónica con D-005.
- Una conversión PCM genérica puede residir en una capa nativa reutilizable de
  audio cuando la arquitectura real lo justifique.
- Otros consumidores futuros no tendrían que depender de `tclwhisper` para
  una transformación PCM genérica.
- `s16le` puede reconsiderarse sin rediscutir desde cero su posible
  representación y escala.
- Slice 5 queda libre para seleccionar otra capacidad con evidencia
  operacional más fuerte.

## D-009 — Exponer `initial_prompt` como opción per-call de `whisper::transcribe`

**Status:** approved

### Question

¿Debe Slice 5 exponer el `whisper_full_params.initial_prompt` de upstream como
opción por llamada, sin trasladar la construcción ni administración de prompts
al binding?

### Evidence

La characterization de `experiments/initial_prompt/` comparó cinco grabaciones
de la voz real del operador con y sin un mismo prompt contextual, usando
`ggml-small.bin`, idioma `spanish` y `n_threads=4`. Los cinco controles sin
prompt reprodujeron sus baselines. En `test5`, el prompt corrigió `BFR` a
`VFR` y `Herring` a `heading`; las frases de control `test2`–`test4` no
mostraron degradaciones relevantes. El cambio al inicio de `test1` no puede
clasificarse sin la referencia exacta de lo pronunciado. Una corrida por
condición y un solo modelo no establecen una mejora general.

En whisper.cpp v1.9.4, `initial_prompt` es un `const char *` en los parámetros
de la llamada; su default es `nullptr`. `no_context=true` y
`carry_initial_prompt=false` son los defaults conservados por el binding.
Con `carry_initial_prompt=false`, upstream incorpora los tokens del prompt al
contexto dinámico; no conserva una copia estática separada para reinyectarla
explícitamente en cada ventana. El historial dinámico puede evolucionar
durante la llamada. Esta evidencia no establece una semántica particular para
una sola ventana interna.

### Decision

Slice 5 expondrá `initial_prompt` como opción per-call de
`whisper::transcribe`. La firma prevista es:

```tcl
whisper::transcribe $handle $pcm \
    ?-language language|auto? \
    ?-n_threads integer? \
    ?-initial_prompt text?
```

La omisión y `-initial_prompt ""` serán equivalentes: en ambos casos se
asignará `params.initial_prompt = nullptr`. Para texto no vacío, el puntero
será válido durante toda la llamada síncrona a `whisper_full()`. No habrá
persistencia del prompt en `WhisperHandle`. `no_context=true` y
`carry_initial_prompt=false` permanecerán sin cambios.

La construcción del prompt corresponde al orquestador Tcl de Iik’, no a
`tclwhisper`.

### Exclusions

Esta decisión no aprueba exponer ni implementar:

- `carry_initial_prompt`, `prompt_tokens`, `prompt_n_tokens` ni `no_context`
  como opciones;
- persistencia en el handle o estado global;
- prompt automático ni concatenación automática de prompts;
- conocimiento aeronáutico en C, normalización semántica ni conversión
  NATO → identificador;
- administración de prompts por misión dentro de `tclwhisper`.

### Further characterization

Sin bloquear Slice 5, queda por caracterizar audio de más de 30 s con un
término de dominio en una ventana posterior, la interacción de
`-initial_prompt` con `-language auto`, y prompts genéricos frente a prompts
específicos por misión. La construcción de estos últimos pertenece al
orquestador Tcl de Iik’.
