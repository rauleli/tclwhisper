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
