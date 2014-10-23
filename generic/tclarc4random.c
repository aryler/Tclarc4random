/*
 * Copyright (c) 2014 Stuart Cassoff <stwo@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * tclarc4random.c
 *
 * Tcl interface to arc4random(3).
 *
 */


#ifdef __cplusplus
extern "C" {
#endif


#define MIN_TCL_VERSION "8.5"


#include <stdlib.h>  /* arc4random(3) */
#include <tcl.h>


static const char *Ta4r = "::arc4random";

static int Ta4r_Random_Cmd  (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
static int Ta4r_Bytes_Cmd   (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
static int Ta4r_Uniform_Cmd (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);

typedef struct {
	const char *name;
	Tcl_ObjCmdProc *proc;
} Ta4r_Cmd;

static Ta4r_Cmd Ta4r_Cmds[] = {
	{ "arc4random"  , Ta4r_Random_Cmd  },
	{ "arc4bytes"   , Ta4r_Bytes_Cmd   },
	{ "arc4uniform" , Ta4r_Uniform_Cmd },
	{ NULL          , NULL             }
};


/***/


static int
Ta4r_Random_Cmd (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
	if (objc != 1) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return TCL_ERROR;
	}

	Tcl_SetObjResult(interp, Tcl_NewWideIntObj(arc4random()));

	return TCL_OK;
}


static int
Ta4r_Bytes_Cmd (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
	int nbytes;
	Tcl_Obj *o;
	unsigned char *buf;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "nbytes");
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[1], &nbytes) != TCL_OK) {
		return TCL_ERROR;
	}
	if (nbytes < 1) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad value \"%d\" for nbytes: must be > 0", nbytes));
		return TCL_ERROR;
	}

	o = Tcl_NewByteArrayObj(NULL, 0);
	Tcl_IncrRefCount(o);
	buf = Tcl_SetByteArrayLength(o, nbytes);

	arc4random_buf(buf, nbytes);

	Tcl_SetObjResult(interp, o);
	Tcl_DecrRefCount(o);

	return TCL_OK;
}


static int
Ta4r_Uniform_Cmd (ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
	int ubound;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "upperbound");
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[1], &ubound) != TCL_OK) {
		return TCL_ERROR;
	}
	if (ubound < 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad value \"%d\" for upperbound: must be >= 0", ubound));
		return TCL_ERROR;
	}

	Tcl_SetObjResult(interp, Tcl_NewWideIntObj(arc4random_uniform(ubound)));

	return TCL_OK;
}


/***/


static int
Ta4r_PackageProvide (Tcl_Interp *interp) {
	return Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION);
}


static int
Ta4r_PackageInit (Tcl_Interp *interp) {
	Tcl_Namespace *ns;
	Ta4r_Cmd *c;
	Tcl_Obj *o;
	Tcl_Obj *m;
	Tcl_Obj *f;

	if ((ns = Tcl_FindNamespace(interp, Ta4r, NULL, TCL_LEAVE_ERR_MSG)) == NULL) { return TCL_ERROR; }

	m = Tcl_NewDictObj();
	for (c = &Ta4r_Cmds[0]; c->name != NULL; c++) {
		/* Put commands into sub-namespace so as not to conflict with ensemble name */
		/* This will also create the sub-namespace. Slightly cheap? */
		o = Tcl_ObjPrintf("%s::commands::%s", Ta4r, c->name);
		Tcl_IncrRefCount(o);
		f = Tcl_ObjPrintf("::tcl::mathfunc::%s", c->name);
		Tcl_IncrRefCount(f);
		if (Tcl_CreateObjCommand(interp, Tcl_GetString(o), c->proc, NULL, NULL) == NULL) {
			Tcl_DecrRefCount(o);
			Tcl_DecrRefCount(f);
			return TCL_ERROR;
		}
		if (Tcl_CreateAlias(interp, Tcl_GetString(f), interp, Tcl_GetString(o), 0, NULL) != TCL_OK) {
			Tcl_DecrRefCount(o);
			Tcl_DecrRefCount(f);
			return TCL_ERROR;
		}
		Tcl_DictObjPut(interp, m, Tcl_NewStringObj(c->name+4, -1), o);
		Tcl_DecrRefCount(o);
		Tcl_DecrRefCount(f);
	}
	if (Tcl_SetEnsembleMappingDict(interp,
		Tcl_CreateEnsemble(interp, (Ta4r+2), ns, TCL_ENSEMBLE_PREFIX), m) != TCL_OK) { return TCL_ERROR; };

	if (Tcl_Export(interp, ns, (Ta4r+2), 0) != TCL_OK) { return TCL_ERROR; }

	return TCL_OK;
}


static int
Ta4r_CommonInit (Tcl_Interp *interp) {
	if (Tcl_InitStubs       (interp, MIN_TCL_VERSION, 0)        == NULL) { return TCL_ERROR; }
	if (Tcl_PkgRequire      (interp, "Tcl", MIN_TCL_VERSION, 0) == NULL) { return TCL_ERROR; }
	if (Tcl_CreateNamespace (interp, Ta4r, NULL, NULL)       == NULL) { return TCL_ERROR; }
	return TCL_OK;
}


EXTERN int
Ta4r_Init (Tcl_Interp *interp) {
	if (Ta4r_CommonInit     (interp) != TCL_OK) { return TCL_ERROR; }
	if (Ta4r_PackageInit    (interp) != TCL_OK) { return TCL_ERROR; }
	if (Ta4r_PackageProvide (interp) != TCL_OK) { return TCL_ERROR; }
	return TCL_OK;
}


EXTERN int
Ta4r_SafeInit (Tcl_Interp *interp) {
	return Ta4r_Init(interp);
}


#ifdef __cplusplus
}
#endif


/* EOF */
