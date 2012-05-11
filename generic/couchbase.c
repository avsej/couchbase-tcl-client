/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "couchbase.h"

/*
 *----------------------------------------------------------------------
 *
 * Create --
 *
 *   Implements the new Tcl "couchbase::create" command.
 *
 * Results:
 *  A standard Tcl result
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
Create_Cmd(
           ClientData clientData,   /* Not used. */
           Tcl_Interp *interp,      /* Current interpreter */
           int argc,                /* Number of arguments */
           Tcl_Obj *const argv[]    /* Argument strings */
)
{
    static const char *options[] = {
        "-hostname", "-port", "-pool", "-bucket", "-username", "-password", NULL
    };
    enum ShaOpts {
        CBOPT_HOSTNAME, CBOPT_PORT, CBOPT_POOL, CBOPT_BUCKET, CBOPT_USERNAME, CBOPT_PASSWORD
    };
    Tcl_Obj *hostname_obj = NULL;
    Tcl_Obj *port_obj = NULL;
    Tcl_Obj *pool_obj = NULL;
    Tcl_Obj *bucket_obj = NULL;
    Tcl_Obj *username_obj = NULL;
    Tcl_Obj *password_obj = NULL;
    int a;

    for (a = 1; a < argc; a++) {
        int index;

        if (Tcl_GetIndexFromObj(interp, argv[a], options, "option", 0, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        if (++a >= argc) {
            goto wrongArgs;
        }
        switch ((enum ShaOpts) index) {
        case CBOPT_HOSTNAME:
            hostname_obj = argv[a];
            continue;
        case CBOPT_PORT:
            port_obj = argv[a];
            continue;
        case CBOPT_POOL:
            pool_obj = argv[a];
            continue;
        case CBOPT_BUCKET:
            bucket_obj = argv[a];
            continue;
        case CBOPT_USERNAME:
            username_obj = argv[a];
            continue;
        case CBOPT_PASSWORD:
            password_obj = argv[a];
            continue;
        }
    }

    {
        char *hostname, *pool, *bucket, *username = NULL, *password = NULL;
        int port = 8091;

        if (hostname_obj != NULL) {
            hostname = Tcl_GetStringFromObj(hostname_obj, NULL);
        } else {
            hostname = strdup("localhost");
        }
        if (port_obj != NULL) {
            if (Tcl_GetIntFromObj(interp, port_obj, &port) != TCL_OK) {
                goto wrongArgs;
            }
        }
        if (pool_obj != NULL) {
            pool = Tcl_GetStringFromObj(pool_obj, NULL);
        } else {
            pool = strdup("default");
        }
        if (bucket_obj != NULL) {
            bucket = Tcl_GetStringFromObj(bucket_obj, NULL);
        } else {
            bucket = strdup("default");
        }
        if (username_obj != NULL) {
            username = Tcl_GetStringFromObj(username_obj, NULL);
        }
        if (password_obj != NULL) {
            password = Tcl_GetStringFromObj(password_obj, NULL);
        }

        fprintf(stderr, "connecting to http://%s:%d/pools/%s/buckets/%s/\n",
                hostname, port, pool, bucket);
        if (username != NULL || password != NULL) {
            fprintf(stderr, "auth: \n");
            if (username != NULL) {
                fprintf(stderr, "\tusername: %s\n", username);
            }
            if (password != NULL) {
                fprintf(stderr, "\tpassword: %s\n", password);
            }
            fprintf(stderr, "\n");
        }
    }

    return TCL_OK;

wrongArgs:
    Tcl_AppendResult(interp, "wrong # args: should be either:\n",
                     "  ",
                     Tcl_GetString(argv[0]),
                     "  ?-hostname hostname? ?-port num?\n",
                     "      ?-pool poolname? ?-bucket bucketname?\n",
                     "      ?-username username? ?-password password?\n",
                     (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Couchbase_Init --
 *
 *  Initialize the new package.
 *
 * Results:
 *  A standard Tcl result
 *
 * Side effects:
 *  The Couchbase package is created.
 *  One new command "sha1" is added to the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */

int
Couchbase_Init(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "couchbase::connect", (Tcl_ObjCmdProc *) Create_Cmd,
                         (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
