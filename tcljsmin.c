/* tcljsmin.c

tcl interface to jsminlib
*/

#include "jsminlib.h"
#include "tcl.h"

typedef struct tcljsminCtx {
	Tcl_Interp *interp;
	char *in;
	int len;
	int pos;
	Tcl_DString out;
	Tcl_DString err;
} tcljsminCtx;

static int tcljsminInput(void *clientData) {
	tcljsminCtx *ctx=clientData;
	if (ctx->pos >= ctx->len) {
		return EOF;
	} else {
		return ctx->in[ctx->pos++];
	}
}

static int tcljsminOutput(void *clientData, char *text, int len) {
	tcljsminCtx *ctx=clientData;
	Tcl_DStringAppend(&ctx->out, text, len);
}

static int tcljsminError(void *clientData, char *text, int len) {
	tcljsminCtx *ctx=clientData;
	Tcl_DStringAppend(&ctx->err, text, len);
}

extern int JSMinCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	int res;
	tcljsminCtx *ctx;
	jsminCtx *jctx;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "Usage: jsmin string");
		return TCL_ERROR;
	}

	ctx=ckalloc(sizeof(tcljsminCtx));

	ctx->in = Tcl_GetStringFromObj(objv[1], &ctx->len);
	ctx->pos = 0;

	Tcl_DStringInit(&ctx->out);
	Tcl_DStringInit(&ctx->err);

	jctx=jsminInit(ctx, tcljsminInput, tcljsminOutput, tcljsminError);

	switch (jsmin(jctx)) {
		case JSMIN_ERROR: 
			Tcl_DStringResult(interp, &ctx->err);
			res=TCL_ERROR;
			break;
		case JSMIN_OK:
			Tcl_DStringResult(interp, &ctx->out);
			res=TCL_OK;
			break;
	}

	jsminCleanup(jctx);

	Tcl_DStringFree(&ctx->out);
	Tcl_DStringFree(&ctx->err);
	ckfree(ctx);
	return res;
}

extern int
Jsmin_Init(Tcl_Interp *interp) {
	Tcl_CreateObjCommand(interp, "jsmin", JSMinCmd, NULL, NULL);
	return TCL_OK;
}

#ifdef NAVISERVER

#include "ns.h"
static int AddCmds(Tcl_Interp *interp, void *arg) {
	Jsmin_Init(interp);
	return NS_OK;
}

NS_EXPORT int Ns_ModuleVersion = 1;
NS_EXPORT int
Ns_ModuleInit(char *server, char *module)
{
	Ns_Log(Notice,"loading jsmin");
	Ns_TclRegisterTrace(server, AddCmds, NULL, NS_TCL_TRACE_CREATE);
	return NS_OK;
}

#endif
