#ifndef _MOD_MODULE_INCLUDED_
#define _MOD_MODULE_INCLUDED_

#include "typedef.h"
#include <utils_ptrlist.h>

cBaseModule* CORE_find_module( int type, const char *name );
PtrList* CORE_get_module_list();
void CORE_unload_modules();
int core_load_module( const char * module_path);




#endif
