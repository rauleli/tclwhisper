#include <tcl.h>
#include <whisper.h>

#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if CHAR_BIT != 8
#error "tclwhisper requires 8-bit bytes for f32le PCM"
#endif

_Static_assert(sizeof(float) == 4,
    "tclwhisper requires 32-bit float for f32le PCM");
_Static_assert(
    FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
    "tclwhisper requires IEEE 754 binary32 float for f32le PCM");

#define WHISPER_ASSOC_KEY "tclwhisper.handleIdentity"

typedef struct WhisperInterpState {
    long instanceSeconds;
    long instanceMicroseconds;
    uint64_t nextHandleId;
    int identitySpaceExhausted;
} WhisperInterpState;

typedef struct WhisperHandle {
    struct whisper_context *context;
    Tcl_Command command;
} WhisperHandle;

static int WhisperHandleCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[]);

static void
WhisperInterpStateDelete(void *clientData, Tcl_Interp *interp)
{
    (void) interp;
    ckfree(clientData);
}

static int
WhisperNextHandleName(
    Tcl_Interp *interp,
    WhisperInterpState *state,
    char *commandName,
    size_t commandNameSize)
{
    uint64_t id;

    while (!state->identitySpaceExhausted) {
        id = state->nextHandleId;
        if (id == UINT64_MAX) {
            state->identitySpaceExhausted = 1;
        } else {
            state->nextHandleId++;
        }

        snprintf(commandName, commandNameSize,
            "::whisper::context%ld_%06ld_%" PRIu64,
            state->instanceSeconds, state->instanceMicroseconds, id);
        if (Tcl_FindCommand(interp, commandName, NULL, TCL_GLOBAL_ONLY) == NULL) {
            return TCL_OK;
        }
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        "tclwhisper handle identity space exhausted", -1));
    Tcl_SetErrorCode(interp, "TCLWHISPER", "HANDLE", "IDENTITY_EXHAUSTED",
        NULL);
    return TCL_ERROR;
}

static void
WhisperHandleDelete(void *clientData)
{
    WhisperHandle *handle = (WhisperHandle *) clientData;

    if (handle->context != NULL) {
        whisper_free(handle->context);
        handle->context = NULL;
    }

    ckfree(handle);
}

static int
WhisperInvalidHandle(Tcl_Interp *interp)
{
    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid tclwhisper handle", -1));
    Tcl_SetErrorCode(interp, "TCLWHISPER", "HANDLE", "INVALID", NULL);
    return TCL_ERROR;
}

static int
WhisperGetHandle(
    Tcl_Interp *interp,
    Tcl_Obj *nameObj,
    Tcl_Command *commandPtr,
    WhisperHandle **handlePtr)
{
    Tcl_Command command;
    Tcl_CmdInfo info;
    WhisperHandle *handle;

    command = Tcl_GetCommandFromObj(interp, nameObj);
    if (command == NULL || !Tcl_GetCommandInfoFromToken(command, &info)) {
        return WhisperInvalidHandle(interp);
    }

    if (!info.isNativeObjectProc ||
            info.objProc != WhisperHandleCmd ||
            info.deleteProc != WhisperHandleDelete ||
            info.objClientData == NULL ||
            info.deleteData != info.objClientData) {
        return WhisperInvalidHandle(interp);
    }

    handle = (WhisperHandle *) info.objClientData;
    if (handle->command != command || handle->context == NULL) {
        return WhisperInvalidHandle(interp);
    }

    if (commandPtr != NULL) {
        *commandPtr = command;
    }
    if (handlePtr != NULL) {
        *handlePtr = handle;
    }
    return TCL_OK;
}

static int
WhisperHandleCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    (void) clientData;
    (void) objc;
    (void) objv;

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        "tclwhisper handles are not directly invocable", -1));
    Tcl_SetErrorCode(interp, "TCLWHISPER", "HANDLE", "NOT_INVOKABLE", NULL);
    return TCL_ERROR;
}

static int
WhisperVersionCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    (void) clientData;

    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(whisper_version(), -1));
    return TCL_OK;
}

static int
WhisperInitCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    WhisperInterpState *interpState;
    struct whisper_context_params params;
    struct whisper_context *context;
    WhisperHandle *handle;
    Tcl_Obj *result;
    char commandName[128];

    (void) clientData;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "model");
        return TCL_ERROR;
    }

    interpState = (WhisperInterpState *) Tcl_GetAssocData(
        interp, WHISPER_ASSOC_KEY, NULL);
    if (interpState == NULL) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "tclwhisper internal interpreter state is unavailable", -1));
        Tcl_SetErrorCode(
            interp, "TCLWHISPER", "INTERNAL", "STATE_UNAVAILABLE", NULL);
        return TCL_ERROR;
    }

    if (WhisperNextHandleName(
            interp, interpState, commandName, sizeof(commandName)) != TCL_OK) {
        return TCL_ERROR;
    }

    params = whisper_context_default_params();
    context = whisper_init_from_file_with_params(Tcl_GetString(objv[1]), params);
    if (context == NULL) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
            "failed to initialize whisper context from model \"%s\"",
            Tcl_GetString(objv[1])));
        Tcl_SetErrorCode(interp, "TCLWHISPER", "INIT", "FAILED", NULL);
        return TCL_ERROR;
    }

    handle = (WhisperHandle *) ckalloc(sizeof(*handle));
    handle->context = context;
    handle->command = NULL;

    handle->command = Tcl_CreateObjCommand(
        interp, commandName, WhisperHandleCmd, handle, WhisperHandleDelete);
    if (handle->command == NULL) {
        WhisperHandleDelete(handle);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "failed to create tclwhisper handle", -1));
        Tcl_SetErrorCode(interp, "TCLWHISPER", "HANDLE", "CREATE_FAILED", NULL);
        return TCL_ERROR;
    }

    result = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, handle->command, result);
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

static int
WhisperFreeCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tcl_Command command;

    (void) clientData;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "handle");
        return TCL_ERROR;
    }

    if (WhisperGetHandle(interp, objv[1], &command, NULL) != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_DeleteCommandFromToken(interp, command);
    return TCL_OK;
}

static int
WhisperTranscribeCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    WhisperHandle *handle;
    unsigned char *pcm;
    int byteLength;
    int nSamples;
    float *samples;
    struct whisper_full_params params;
    int whisperResult;
    int nSegments;
    int i;
    Tcl_Obj *result;

    (void) clientData;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "handle pcm");
        return TCL_ERROR;
    }

    if (WhisperGetHandle(interp, objv[1], NULL, &handle) != TCL_OK) {
        return TCL_ERROR;
    }

    pcm = Tcl_GetByteArrayFromObj(objv[2], &byteLength);
    if (byteLength % (int) sizeof(float) != 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "PCM f32le input requires complete 4-byte samples", -1));
        Tcl_SetErrorCode(
            interp, "TCLWHISPER", "PCM", "INVALID_LENGTH", NULL);
        return TCL_ERROR;
    }

    if (byteLength == 0) {
        Tcl_SetObjResult(interp, Tcl_NewObj());
        return TCL_OK;
    }

    nSamples = byteLength / (int) sizeof(float);
    samples = (float *) ckalloc((size_t) nSamples * sizeof(*samples));
    for (i = 0; i < nSamples; i++) {
        uint32_t bits =
            ((uint32_t) pcm[4*i + 0]      ) |
            ((uint32_t) pcm[4*i + 1] <<  8) |
            ((uint32_t) pcm[4*i + 2] << 16) |
            ((uint32_t) pcm[4*i + 3] << 24);

        memcpy(&samples[i], &bits, sizeof(bits));
    }

    params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.print_special = false;

    whisperResult = whisper_full(
        handle->context, params, samples, nSamples);
    if (whisperResult != 0) {
        ckfree(samples);
        Tcl_SetObjResult(interp, Tcl_ObjPrintf(
            "whisper_full failed with code %d", whisperResult));
        Tcl_SetErrorCode(
            interp, "TCLWHISPER", "TRANSCRIBE", "FAILED", NULL);
        return TCL_ERROR;
    }

    result = Tcl_NewObj();
    nSegments = whisper_full_n_segments(handle->context);
    for (i = 0; i < nSegments; i++) {
        Tcl_AppendToObj(
            result, whisper_full_get_segment_text(handle->context, i), -1);
    }

    ckfree(samples);
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

DLLEXPORT int
Tclwhisper_Init(Tcl_Interp *interp)
{
    WhisperInterpState *interpState;
    Tcl_Time now;

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return TCL_ERROR;
    }

    if (Tcl_CreateNamespace(interp, "whisper", NULL, NULL) == NULL) {
        return TCL_ERROR;
    }

    interpState = (WhisperInterpState *) ckalloc(sizeof(*interpState));
    Tcl_GetTime(&now);
    interpState->instanceSeconds = now.sec;
    interpState->instanceMicroseconds = now.usec;
    interpState->nextHandleId = 1;
    interpState->identitySpaceExhausted = 0;
    Tcl_SetAssocData(
        interp, WHISPER_ASSOC_KEY, WhisperInterpStateDelete, interpState);

    Tcl_CreateObjCommand(
        interp, "whisper::version", WhisperVersionCmd, NULL, NULL);
    Tcl_CreateObjCommand(
        interp, "whisper::init", WhisperInitCmd, NULL, NULL);
    Tcl_CreateObjCommand(
        interp, "whisper::free", WhisperFreeCmd, NULL, NULL);
    Tcl_CreateObjCommand(
        interp, "whisper::transcribe", WhisperTranscribeCmd, NULL, NULL);

    return Tcl_PkgProvide(interp, "tclwhisper", "0.1");
}
