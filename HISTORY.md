# HISTORY — tclwhisper

Este documento registra cómo se llegó al diseño y al inicio de implementación de `tclwhisper`, el binding Tcl de `whisper.cpp` destinado al subsistema STT de Iik’.

No pretende sustituir a `DECISIONS.md`.

`DECISIONS.md` registra decisiones concretas y sus consecuencias. Este archivo conserva la historia del razonamiento, las alternativas consideradas y la participación del cónclave técnico.

---

## Origen

Dentro de la arquitectura de Iik’, el reconocimiento de voz se separó explícitamente de la adquisición y normalización de audio.

La cadena conceptual quedó planteada como:

```text
adquisición
    ↓
normalización PCM
    ↓
tclwhisper
    ↓
whisper.cpp
    ↓
texto
```

`tclwhisper` no debe conocer micrófonos, ALSA, dispositivos físicos, archivos WAV, MP3 ni mecanismos de reproducción.

Su responsabilidad es exponer a Tcl la capacidad nativa de `whisper.cpp`.

El principio adoptado fue:

> El binding expone una capacidad nativa; Tcl decide cómo conectarla dentro de Iik’.

---

## Propuesta inicial de Claude

Claude propuso inicialmente una API pequeña inspirada en el estilo que se atribuía entonces a `tclllama`:

```tcl
package require tclwhisper

whisper init /modelos/ggml-small.bin -n_threads 4 -use_gpu 1
set texto [whisper transcribe $pcm -format s16le -rate 16000 -language es]
whisper free
```

La propuesta incluía:

* `init`;
* `transcribe`;
* `info`;
* `version`;
* `free`;
* `s16le` y `f32le`;
* sample rate de 16 kHz;
* selección de idioma;
* traducción;
* beam search;
* temperatura;
* initial prompt;
* resultados detallados por segmentos.

Claude propuso además reutilizar el esqueleto TEA de `tclllama` y enlazar contra una `libwhisper` compartida.

La propuesta fue tomada como candidato para discusión, no como especificación aprobada.

---

## Primera revisión de DeepSeek

DeepSeek estuvo de acuerdo con dos decisiones estructurales importantes:

* `transcribe` podía ser bloqueante en esta etapa;
* la decodificación, resampling y preparación del audio debían permanecer fuera de `tclwhisper`.

Después señaló varias tensiones que la propuesta inicial había dejado implícitas.

Entre ellas:

* si `s16le` debía realmente convertirse dentro del binding;
* si singleton era una restricción deliberada;
* cómo representar segmentos y resultados enriquecidos;
* qué ocurriría con buffers largos;
* cómo construir y enlazar `whisper.cpp`;
* qué política de errores Tcl utilizar;
* cómo manejar el logging nativo;
* qué hacer en el futuro si fuera necesaria cancelación.

DeepSeek apoyó además una implementación incremental:

```text
TEA + version
→ lifecycle
→ transcribe
→ resultados enriquecidos
```

Esto reforzó la idea de no desarrollar toda la API simultáneamente.

---

## Inspección de tclllama

La discusión produjo un hallazgo importante.

La premisa de que `tclllama` era singleton resultó no corresponder con su implementación actual.

El código vigente de `tclllama`:

1. reserva un estado durante `llama::init`;
2. crea un handle asociado a ese estado;
3. devuelve dicho handle a Tcl;
4. requiere posteriormente ese handle para operaciones como generación y liberación.

Por tanto, la implementación real de `tclllama` es multi-instancia y handle-based.

Parte de su documentación conserva todavía ejemplos correspondientes a una API anterior.

Esto eliminó una de las principales justificaciones para adoptar singleton automáticamente en `tclwhisper`.

La cuestión singleton frente a handles quedó nuevamente abierta para una decisión posterior basada en necesidades reales de STT.

---

## Inspección de whisper.cpp

La API nativa mostró también que varias decisiones propuestas originalmente pertenecían a capas diferentes.

`whisper.cpp` separa la configuración de creación del contexto de los parámetros utilizados durante una transcripción.

También consume directamente muestras PCM en representación floating point y establece 16 kHz como frecuencia de trabajo del motor.

Esto llevó a cuestionar que opciones como:

```text
-rate 16000
```

deban existir necesariamente en Tcl.

Si un byte array no contiene metadatos de sample rate, el binding no puede comprobar que las muestras realmente sean de 16 kHz; solamente podría comprobar que el llamante escribió el número esperado.

De manera similar, aceptar `s16le` y convertirlo internamente a floating point sería una capacidad agregada por `tclwhisper`, no una exigencia de `whisper.cpp`.

La decisión sobre esa comodidad quedó pospuesta hasta conocer la ruta de audio real.

También se observó que greedy y beam search son estrategias distintas en upstream. Por ello no se quiso congelar una abstracción donde un determinado `beam_size` representara implícitamente greedy.

---

## Discusión sobre resultados detallados

Claude y DeepSeek propusieron devolver, opcionalmente, información como:

* texto completo;
* idioma;
* nombre del modelo;
* duración;
* segmentos;
* timestamps.

La información fue considerada potencialmente útil para trazabilidad y logging.

Sin embargo, se decidió no fijar todavía la representación Tcl concreta.

La forma del resultado debe diseñarse cuando exista una transcripción funcional y pueda evaluarse contra sus consumidores reales.

---

## Build y selección de upstream

Durante el cónclave se decidió trabajar inicialmente contra una release concreta de `whisper.cpp`.

La versión seleccionada fue:

```text
v1.9.4
```

El source de `whisper.cpp` se mantiene fuera del repositorio `tclwhisper`.

La organización preferida es conceptualmente:

```text
workspace/
├── tclwhisper/
├── whisper.cpp-v1.9.4/
├── whisper-build-v1.9.4/
└── whisper-install-v1.9.4/
```

Esto evita vendorizar upstream y permite probar `tclwhisper` contra una instalación real de su dependencia.

En la máquina de desarrollo, el clone y la compilación de `whisper.cpp` fueron realizados antes de comenzar el primer slice de `tclwhisper`.

---

## Cambio de enfoque

Después de las iteraciones de Claude, DeepSeek y ChatGPT surgió consenso sobre un punto fundamental:

**no era necesario aprobar toda la API antes de comenzar a producir evidencia.**

La primera implementación podía ser mucho más pequeña.

En vez de construir inmediatamente:

```text
init
transcribe
info
verbose
free
formatos PCM
GPU
segments
```

se decidió comprobar primero únicamente que Tcl pudiera cargar una extensión propia enlazada contra `whisper.cpp`.

Así nació el Slice 0.

---

## Slice 0

El objetivo aprobado es únicamente:

```tcl
package require tclwhisper
puts [whisper::version]
```

La extensión debe obtener la versión mediante la función nativa:

```c
whisper_version()
```

Durante este trabajo también debe caracterizarse la cadena real de build y runtime:

```text
Tcl
 ↓
tclwhisper.so
 ↓
libwhisper.so
 ↓
GGML y demás dependencias
```

El slice debe determinar mediante observación:

* qué headers se requieren;
* cómo descubrir `libwhisper`;
* qué aporta `pkg-config`;
* qué bibliotecas aparecen como dependencias;
* cómo debe resolverse el runtime;
* qué versión exacta reporta la biblioteca compilada.

No debe avanzar a carga de modelos ni transcripción.

---

## Papel de Codex

Codex recibe deliberadamente un papel de ejecutor e inspector.

En este proceso su responsabilidad es:

* inspeccionar;
* establecer el baseline;
* construir;
* ejecutar pruebas;
* registrar resultados;
* distinguir hechos de inferencias;
* reportar discrepancias.

Las decisiones arquitectónicas importantes regresan al cónclave.

La secuencia de trabajo acordada queda así:

```text
cónclave
   ↓
pregunta concreta
   ↓
Codex inspecciona / experimenta
   ↓
evidencia
   ↓
cónclave revisa
   ↓
Raúl decide
   ↓
siguiente slice
```

---

## Estado actual

Al momento de escribir este documento:

* `whisper.cpp` ya fue clonado;
* upstream ya fue compilado en la máquina de desarrollo;
* Codex está trabajando en el Slice 0 de `tclwhisper`;
* la API completa de `tclwhisper` permanece deliberadamente sin congelar;
* el siguiente slice se decidirá únicamente después de revisar la evidencia producida por este primer build.

El objetivo inmediato no es todavía transcribir audio.

El objetivo inmediato es demostrar, con el mínimo código posible, que la frontera Tcl → `tclwhisper` → `whisper.cpp` está correctamente construida.
