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
