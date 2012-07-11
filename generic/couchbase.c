/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "couchbase.h"

#define COUCHBASE_NAME_LENGTH 11 + sizeof(void*) * 2 + 1
#define COUCHBASE_NAME_TEMPLATE "couchbase%p"

typedef struct Couchbase_Connection {
    const char *connName;
} Couchbase_Connection;

static void
Delete_Registry(ClientData clientData,  /* The per-interpreter data structure. */
                Tcl_Interp *interp)     /* The interpreter being deleted. */
{
    Tcl_HashTable *registry;    /* Hash table of channels. */
    Tcl_HashSearch search;      /* Search variable. */
    Tcl_HashEntry *entry;
    Couchbase_Connection *conn;

    registry = clientData;
    for (entry = Tcl_FirstHashEntry(registry, &search); entry != NULL;
         entry = Tcl_FirstHashEntry(registry, &search)) {
        conn = Tcl_GetHashValue(entry);
        ckfree(conn);
        Tcl_DeleteHashEntry(entry);
    }
    Tcl_DeleteHashTable(registry);
    ckfree(registry);
}

static int
Couchbase_GetFromObj(Tcl_Interp *interp,
                     Tcl_Obj *obj,
                     Couchbase_Connection *conn)
{
/* достать какой-то тип из объекта, бла-бла-бла */
}

static void
Couchbase_Register(Tcl_Interp *interp,         /* Interpreter in which to add the channel. */
                   Couchbase_Connection *conn) /* The connection to add to this interpreter
                                                * registry. */
{
    Tcl_HashTable *registry;    /* Hash table of channels. */
    Tcl_HashEntry *entry;       /* Search variable. */
    int isNew;                  /* Is the hash entry new or does it exist? */

    /* register the connection descriptor */
    registry = Tcl_GetAssocData(interp, "couchbase", NULL);
    if (registry == NULL) {
        registry = ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(registry, TCL_STRING_KEYS);
        Tcl_SetAssocData(interp, "couchbase", Delete_Registry, registry);
    }
    entry = Tcl_CreateHashEntry(registry, conn->connName, &isNew);
    if (!isNew) {
        if (conn == Tcl_GetHashValue(entry)) {
            return;
        }
        Tcl_Panic("Couchbase_Register: duplicate connection names");
    }
    Tcl_SetHashValue(entry, conn);
}

static int
Couchbase_Get(ClientData clientData,   /* Not used. */
              Tcl_Interp *interp,      /* Current interpreter */
              int argc,                /* Number of arguments */
              Tcl_Obj *const argv[])   /* Argument strings */
{
    Couchbase_Connection *conn = NULL;
    int ttl = 0, extended = 0;
    char **keys = NULL;
    unsigned int nkeys = 0;

    /* parse options */
    {
        static const char *options[] = {
            "-extended", NULL
        };
        enum GetOpts {
            GET_EXTENDED
        };
        unsigned int a, n;
        int optionIndex;
        for (a = 1; a < argc; ++a) {
            const char *arg = Tcl_GetString(argv[a]);
            if (arg[0] != '-') {
                break;
            }
            if (Tcl_GetIndexFromObj(interp, argv[a], options, "option",
                                    TCL_EXACT, &optionIndex) != TCL_OK) {
                return TCL_ERROR;
            }
            if (++a >= argc) {
                goto wrongNumArgs;
            }
            switch ((enum GetOpts) optionIndex) {
            case GET_EXTENDED:
                if (Tcl_GetBooleanFromObj(interp, argv[a], &extended) != TCL_OK) {
                    Tcl_AppendResult(interp, " for -extended option", NULL);
                    return TCL_ERROR;
                }
                continue;
            }
        }

        if (argc - a < 2) {
wrongNumArgs:
            Tcl_WrongNumArgs(interp, 1, argv, "?-extended bool? conn key ?key ...?");
            return TCL_ERROR;
        }

        if (Couchbase_GetFromObj(interp, argv[a++], &conn) != TCL_OK) {
            return TCL_ERROR;
        }
        nkeys = argc - a;
        keys = ckalloc(nkeys * sizeof(char *));
        for (n = 0; a < argc; ++a, ++n) {
            keys[n] = Tcl_GetString(argv[a]);
        }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Create --
 *
 *   Implements the new Tcl "couchbase" command.
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
Couchbase_Connect(ClientData clientData,   /* Not used. */
                  Tcl_Interp *interp,      /* Current interpreter */
                  int argc,                /* Number of arguments */
                  Tcl_Obj *const argv[])   /* Argument strings */
{
    char *hostname = NULL, *pool = NULL, *bucket = NULL, *username = NULL, *password = NULL;
    int port = 8091;

    /* parse options */
    {
        static const char *options[] = {
            "-hostname", "-port", "-pool", "-bucket", "-username", "-password", NULL
        };
        enum ConnectOpts {
            CONN_HOSTNAME, CONN_PORT, CONN_POOL, CONN_BUCKET, CONN_USERNAME, CONN_PASSWORD
        };
        unsigned int a;
        int optionIndex;
        for (a = 1; a < argc; ++a) {
            const char *arg = Tcl_GetString(argv[a]);
            if (arg[0] != '-') {
                break;
            }
            if (Tcl_GetIndexFromObj(interp, argv[a], options, "option",
                                    TCL_EXACT, &optionIndex) != TCL_OK) {
                return TCL_ERROR;
            }
            if (++a >= argc) {
                goto wrongNumArgs;
            }
            switch ((enum ConnectOpts) optionIndex) {
            case CONN_HOSTNAME:
                hostname = Tcl_GetString(argv[a]);
                continue;
            case CONN_PORT:
                if (Tcl_GetIntFromObj(interp, argv[a], &port) != TCL_OK) {
                    Tcl_AppendResult(interp, " for -port option", NULL);
                    return TCL_ERROR;
                }
                continue;
            case CONN_POOL:
                pool = Tcl_GetString(argv[a]);
                continue;
            case CONN_BUCKET:
                bucket = Tcl_GetString(argv[a]);
                continue;
            case CONN_USERNAME:
                username = Tcl_GetString(argv[a]);
                continue;
            case CONN_PASSWORD:
                password = Tcl_GetString(argv[a]);
                continue;
            }
        }
        if (a < argc) {
wrongNumArgs:
            Tcl_WrongNumArgs(interp, 1, argv,
                             "?-hostname str? ?-port num?  ?-pool str? ?-bucket str?  ?-username str? ?-password str?");
            return TCL_ERROR;
        }
    }

    /*
     * set defaults
     */
    if (hostname == NULL) {
        hostname = strdup("localhost");
    }
    if (pool == NULL) {
        pool = strdup("default");
    }
    if (bucket == NULL) {
        bucket = strdup("default");
    }
    (void)username;
    (void)password;

    /*
     * create connection
     */
    {
        Couchbase_Connection *conn = NULL;
        char connName[COUCHBASE_NAME_LENGTH];

        conn = ckalloc(sizeof(Couchbase_Connection));
        if (conn == NULL) {
            return TCL_ERROR;
        }
        memset(conn, 0, sizeof(Couchbase_Connection));
        sprintf(connName, COUCHBASE_NAME_TEMPLATE, conn);
        conn->connName = strdup(connName);
        Couchbase_Register(interp, conn);
        Tcl_AppendResult(interp, conn->connName, NULL);
    }
    return TCL_OK;
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
 *  One new command "couchbase" is added to the Tcl interpreter.
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
    Tcl_CreateObjCommand(interp, "couchbase::connect", Couchbase_Connect, NULL, NULL);
    Tcl_CreateObjCommand(interp, "couchbase::get", Couchbase_Get, NULL, NULL);

    return TCL_OK;
}
