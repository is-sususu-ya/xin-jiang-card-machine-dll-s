#ifndef UTILS_INI_PRASE
#define UTILS_INI_PRASE

#include "utils_ptrlist.h"
typedef struct tagIniUnit {
	const char *key; // 节点名称
	PtrList items;  // 键值对
	PtrList subunits; 
}IniUnit; 

IniUnit * IniConfig_load(const char * path);

IniUnit* IniConfig_get_subunit( IniUnit *cf, const char *key );
int IniConfig_get_item_as_int( IniUnit *cf, const char *key , int def_value );
const char * IniConfig_get_item_safe(IniUnit * cf, const char *key, const char * def_value);

void IniConfig_terminate(IniUnit * cf);

#endif