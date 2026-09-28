#include <tcl.h>
#include <whisper.h>

#include <stdio.h>

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
    Tcl_Command *commandPtr)
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

    *commandPtr = command;
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
    struct whisper_context_params params;
    struct whisper_context *context;
    WhisperHandle *handle;
    Tcl_Obj *result;
    char commandName[80];

    (void) clientData;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "model");
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

    snprintf(commandName, sizeof(commandName),
        "::whisper::context%p", (void *) handle);
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

    if (WhisperGetHandle(interp, objv[1], &command) != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_DeleteCommandFromToken(interp, command);
    return TCL_OK;
}

DLLEXPORT int
Tclwhisper_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return TCL_ERROR;
    }

    if (Tcl_CreateNamespace(interp, "whisper", NULL, NULL) == NULL) {
        return TCL_ERROR;
    }

    Tcl_CreateObjCommand(
        interp, "whisper::version", WhisperVersionCmd, NULL, NULL);
    Tcl_CreateObjCommand(
        interp, "whisper::init", WhisperInitCmd, NULL, NULL);
    Tcl_CreateObjCommand(
        interp, "whisper::free", WhisperFreeCmd, NULL, NULL);

    return Tcl_PkgProvide(interp, "tclwhisper", "0.1");
}
