/*
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <lc_module.h>
#include <typedef.h>
#include <utils_ptrlist.h>
#include "utils_file.h"

#define DIR_MODULE "/home/lib"

static PtrList m_ModuleList = PTRLIST_INITIALIZER;

void ltrace(const char *fmt, ...);

int core_load_module( const char * module_path)
{
	void 		*dlhandle;
	cIOModule* 	(*func)( void );
	MODULE_INFO_S	*modinfo;
	cBaseModule	*mod;

	dlhandle = dlopen( module_path, RTLD_LAZY );
	if( ! dlhandle ) 
	{
		fprintf(stderr, "%s: %s\n", dlerror(),module_path);
		return -1;
	}

	func = dlsym( dlhandle, "get_module" );
	if( func )
	{
		mod = (cBaseModule	*)(*func)();
		printf("\n<mod=%p><%s><%s><%d>\n",mod,__FILE__,__func__,__LINE__);
		if( mod && (mod->check_size == sizeof(cBaseModule)) )
		{
			if(0 == mod->initialize() )
			{
				modinfo = malloc( sizeof(MODULE_INFO_S) );
				assert(modinfo!=NULL);
				if( modinfo ) 
				{
					memset(modinfo, 0, sizeof(MODULE_INFO_S) );
					modinfo->path = strdup( module_path );
					modinfo->dlh = dlhandle;
					modinfo->mod = mod;
					PtrStack_push( &m_ModuleList, modinfo );

					ltrace( "Module '%s' loaded (build: %d.%d.%d) - %s.\n",
						mod->name,
						Y_FMVER( mod->version ),
						M_FMVER( mod->version ),
						D_FMVER( mod->version ),
						mod-> description
						);
					return 0;
				}
			} 
			else 
				ltrace( "initializing module '%s' failed.\n", mod->name );
		}
		else
			ltrace( "File '%s' is not our loadable module.\n", module_path );
	}
	else
		ltrace("failed to address the entry of 'get_module' in dynamic loading module %s!\n", module_path);
	return -1;
}

void CORE_unload_modules()
{
	PtrList 	*list = &m_ModuleList;
	MODULE_INFO_S	*modinfo;
	//POSITION	pos;
	char		mod_name[ 128 ];
	int year,month,day;
	char mod_description[128];

	ltrace("CORE_unload_modules PtrList_get_count(list)=%d\n",PtrList_get_count(list));
	while( PtrList_get_count(list) > 0 )
	{
		modinfo = PtrStack_pop(list);
		memset(mod_name,0,sizeof(mod_name));
		strcpy( mod_name, modinfo->mod->name );
		memset(mod_description,0,sizeof(mod_description));
		strcpy( mod_description, modinfo->mod->description );
		year = Y_FMVER( modinfo->mod->version );
		month = M_FMVER( modinfo->mod->version );
		day = D_FMVER( modinfo->mod->version );
		
		modinfo->mod->terminate();
		dlclose(modinfo->dlh);
		if( modinfo->path ) free( modinfo->path );
		free( modinfo );
		ltrace( "Module '%s'(%d,%d,%d,%s) unloaded.\n", mod_name,year,month,day,mod_description ); //ltrace( "Module '%s' unloaded.\n", mod_name );
	}
	return 0;
}

PtrList* CORE_get_module_list()
{
	PtrList 	*list = & m_ModuleList;
	return list;
}

cBaseModule* CORE_find_module( int type, const char *name )
{
 	PtrList *list = &m_ModuleList;
 	MODULE_INFO_S *modinfo;
 	POSITION pos;
 	for ( pos=list->head;pos!=NULL;pos=pos->next)
 	{
 		modinfo = (MODULE_INFO_S *)pos->ptr;
 		if(name==NULL)
 		{
	 		if ( modinfo->mod->is_type(type) )
	 		{
	 			return modinfo->mod;
	 		}
	 	}
	 	else 
	 	{
	 		if ( modinfo->mod->is_type(type) && strcmp(modinfo->mod->name ,name)==0)
	 		{
	 			return modinfo->mod;
	 		}
	 	}
 	}
 	return NULL;
}

int CORE_enable_modules( int type, int bEnable )
{
	PtrList 	*list = & m_ModuleList;
	MODULE_INFO_S	*modinfo;
	POSITION	pos;
	int count=0;
	for( pos=list->head; pos!=NULL; pos=pos->next)
	{
		modinfo = (MODULE_INFO_S *)pos->ptr;
		if ( modinfo->mod->is_type(type) )
		{
			((cIOModule *)modinfo->mod)->enable(NULL,bEnable);  // enable这个模块的所有对象
			count++;
		}
	}
	return count;
}
