#include <tcl.h>
#include <whisper.h>

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

    return Tcl_PkgProvide(interp, "tclwhisper", "0.1");
}
