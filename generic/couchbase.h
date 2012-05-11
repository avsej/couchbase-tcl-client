#ifndef _COUCHBASE_H
#define _COUCHBASE_H

#include <tcl.h>

/*
 * Windows needs to know which symbols to export.
 */

#ifdef BUILD_couchbase
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_couchbase */

/*
 * Only the _Init function is exported.
 */

EXTERN int  Couchbase_Init(Tcl_Interp * interp);

#endif /* _COUCHBASE */
