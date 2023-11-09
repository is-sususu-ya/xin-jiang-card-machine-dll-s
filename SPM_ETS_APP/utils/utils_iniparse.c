#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "utils_ptrlist.h"
#include "utils_iniparse.h"

void IniConfig_terminate(IniUnit * cf);
static  void IniConfig_clean(IniUnit * cf);

static IniUnit * IniConfig_new(const char * key ) {
	IniUnit * cf; 

	cf = (IniUnit * )malloc(sizeof( * cf)); 
	if ( ! cf)return NULL; 
	memset(cf, 0, sizeof( * cf)); 

	cf->key = strdup(key); 
	PtrList_initialize( &cf->items); 
	PtrList_initialize( &cf->subunits); 
	return cf; 
}

static  void IniConfig_initialize(IniUnit * cf, const char * key) {
	cf-> key = strdup(key); 
	PtrList_initialize( &cf->items); 
	PtrList_initialize( &cf->subunits); 
}


void IniConfig_terminate(IniUnit * cf) {
	IniConfig_clean(cf); 
	free((void*)cf->key); 
}

static  void IniConfig_clean(IniUnit * cf) {
	StrStrPair * item; 
	IniUnit * subunit; 
	
	while (cf->items.count > 0) {
		item = (StrStrPair * )PtrList_remove_head( &cf->items); 
		free(item-> key); 
		free(item-> value); 
		free(item); 
	}
	
	while (cf-> subunits.count > 0) {
		subunit = (IniUnit *)PtrList_remove_head( &cf->subunits); 
		IniConfig_terminate(subunit); 
		free(subunit); 
	}
}

static void IniConfig_add_item(IniUnit * cf, const char * key, const char * value) {
	StrStrPair * newitem; 
	newitem = (StrStrPair * )malloc(sizeof( * newitem)); 
	if ( ! newitem)return; 

	newitem-> key = strdup(key); 
	newitem-> value = strdup(value); 
	PtrList_append( &cf->items, (void * )newitem); 
}

static void IniConfig_add_subunit(IniUnit * cf, IniUnit * subcf) {
	PtrList_append( & cf-> subunits, (void * )subcf); 
}

IniUnit* IniConfig_get_subunit( IniUnit *cf, const char *key )
{
	IniUnit	*subunit;
	PtrList		*list = & cf->subunits;
	POSITION 	pos;

	for(pos=list->head; pos!=NULL; pos=pos->next ) {
		subunit = (IniUnit*) pos->ptr;
		if( 0 == strcmp(key, subunit->key) ) return subunit;
	}
	return NULL;
}

const char * IniConfig_get_item(IniUnit * cf, const char *key ) {
	StrStrPair * item; 
	PtrList * list =  &cf->items; 
	POSITION  pos; 
	for (pos = list->head; pos != NULL; pos = pos->next) {
		item = (StrStrPair *)pos->ptr; 
		if (0 == strcmp(key, item->key)) {
			return item->value; 
		}
	}
	return NULL; 

}
int IniConfig_get_item_as_int( IniUnit *cf, const char *key, int defval )
{
	const char* strval;
	strval = IniConfig_get_item(cf, key);
	if(! strval) return defval;

	return strtol(strval,NULL,0);
}

int IniConfig_get_item_as_bool( IniUnit *cf, const char *key, int defval )
{
	const char* strval;
	const char* truestr[] = { "true", "yes", "enable", "on", "1" };
	const char* falsestr[] = { "false", "no", "disable", "off", "0" };
	int i;

	strval = IniConfig_get_item(cf, key);
	if(! strval) return defval;

	for(i=0; i<sizeof(truestr)/sizeof(char*); i++) {
		if( 0 == strcasecmp(strval, truestr[i]) ) return 1;
	}

	for(i=0; i<sizeof(truestr)/sizeof(char*); i++) {
		if( 0 == strcasecmp(strval, falsestr[i]) ) return 0;
	}

	return defval;
}

double IniConfig__get_item_as_double( IniUnit *cf, const char *key, double defval )
{
	const char* strval;
	strval = IniConfig_get_item(cf, key);
	if(! strval) return defval;

	return strtod(strval,NULL);
}

void IniConfig_delete(IniUnit * cc) {
	IniConfig_terminate( cc );
	free( cc );
}

int IniConfig_delete_subunit( IniUnit *cf, const char *key )
{
	IniUnit	*subunit;
	PtrList		*list = & cf->subunits;
	POSITION 	pos, pos_this;

	for(pos=list->head; pos!=NULL; )
	{
		subunit = (IniUnit*) pos->ptr;
		pos_this = pos;
		pos = pos->next;
		if( 0 == strcmp(key, subunit->key) )
		{
			PtrList_remove(list, pos_this);
			IniConfig_delete(subunit);
			return 0;
		}
	}
	return -1;
}



// 行末尾肯定是\n 
static int get_line( const char *ptr, char *dst )
{
	const char *start = ptr;
	if( ptr ==NULL || dst == NULL || *ptr == '\0' )
	return 0;
	for( ; *ptr != '\0' && *ptr != '\n'; )
	*dst++ = *ptr++;
	*dst = '\0';
	return ptr - start + 1;
}

static char* trimc( char *str )
{
	int len;
	char *ptr, *q; 
	for(ptr=str; *ptr==' ' || *ptr=='\t'; ptr++);
	for(q=str+strlen(str)-1; *q==' ' || *q=='\t' || *q=='\r' || *q=='\n'; q--);
	len = q+1 - ptr;
	memmove(str, ptr, len);
	str[len] = '\0';
	return str;
}

static int str_trimed( char *str )
{
	char buffer[256] = {0};
	char *ptr = NULL;
	if( strlen( str ) > sizeof( buffer ) ) return;
	strcpy( buffer, str );
	ptr = trimc( buffer );
	strcpy( str, buffer );
}

static int parse_line(const char *arg, char *key, char *value )
{
	// ini格式的字符是以key=value格式的，我们这里不支持多entry的格式，只支持单独的key和value格式，如果
	// 是注释，那么必然是#开头的
	const char *ptr = arg;
	char line[256];
	if( arg == NULL || *arg == '\0' )
	return -1;
	if( strlen( arg ) > sizeof( line ) ) 
	return -1;
	if( strstr( arg, "=" ) == NULL ) return;
	for( ; *ptr == '\r' || *ptr == '\n' || *ptr == ' ' || *ptr == '\t'; ptr++ );
	if( *ptr == '#' || (*ptr == '/'  && *(ptr + 1 ) == '/' || *ptr == ';' ) ) 
	{   
		printf("这是一个注解内容，忽略..\r\n");
		return -1;
	}   
	if( *ptr == '\0' )
	return -1;
	// 注释的部分需要全部去掉
	strcpy( line, arg );
	if( NULL != ( ptr = strstr( line , "#" )) )
	{   
		line[ ptr - line ] = '\0';
	}   
	// 差不多这个时候只剩下key 和value了
	ptr = strstr( line, "=" );
	if( ptr )
	{
		strncpy( key, line, ptr - line );
		strncpy( value, ptr+1, strlen( ptr + 1));
	}
	str_trimed( key );
	str_trimed( value );
	return 0;
}

static int is_entries( const char *text , char *entry )
{
	char *ptr = NULL;
	if(text == NULL || *text == '\0' ) return 0;
	if( *text != '[' ) return 0;
	strcpy( entry, text + 1);
	ptr = entry;
	for( ; *ptr != ']' && *ptr != '\0'; ptr++ );
	if( *ptr == ']' )
	*ptr = '\0';
	else
	return 0; // format error..
	return 1;
}

static void delete_comment( char *text )
{
	if(text == NULL || *text == '\0' ) return;
	for( ; *text != ';' && *text != '#' && ( *text != '/' && *(text+1) != '/') && *text != '\0'; text++ );
	if( *text != '\0' )
	*text = '\0';
}

//这里的话，先去除注释字段
static int str_process_prev( char *text )
{
	char line[256] = {0};
	if(text == NULL || *text == '\0' ) return 0;
	strncpy( line, text, sizeof( line ) );
	str_trimed( line );
	if( line[0] == ';' \
			|| line[0] == '\0' \
			|| line[0] == '#' \
			|| (line[0] == '/' && line[1] == '/' ) )
	return 0;
	delete_comment( line );
	str_trimed( line );
	strcpy( text, line );
	return 1;
}

static int text_valid( const char *text )
{
	if(text == NULL || *text == '\0' ) return 0;
	if( *text == '[' ) return 1;
	if( strstr( text, "=" ) )return 2;
	return 0;
}

static int get_entry( const char *text, char *entry )
{
	const char *ptr = text;
	if( *ptr != '[' ) return -1;
	ptr++;
	for( ;*ptr != ']' && *ptr != '\0'; )
	*entry++ = *ptr++;
	*entry = '\0';
	return 0;
}

static IniUnit *parse_text(IniUnit * cf, const char * text) 
{
	// ini格式文件只有单层配置，没有多层子配置，因此
	char line[256];
	char key[256];
	char value[256];
	int line_len;
	const char *ptr = text;
	int ret;
	if(strlen( text ) <= 0 )
		return NULL;
	IniUnit *root = cf;
	IniUnit *sub = root;
	while( *ptr != '\0' )
	{
		line_len = get_line( ptr, line );
		if( line_len == 0 )
			break;
		ptr += line_len;
		if( line[0] == '\n' || line[0] == '\0' )
		{
			ptr++;
			continue;
		}
		str_process_prev( line );
		ret = text_valid( line );
		if( ret == 0 )
		continue;
		if( ret == 1 )
		{
			if( get_entry( line, key ) == 0 )
			{
				sub = IniConfig_new( key );
				IniConfig_add_subunit( root, sub );
			}
		}
		if( ret == 2 )
		{
			memset( key, 0, sizeof( key ));
			memset( value, 0, sizeof(value ));
			if( parse_line( line, key, value ) == 0 )
			{
				if( key[0] == '\0' || value[0] == '\0' ) continue;
				IniConfig_add_item( sub, key, value );
			}
		}
	}
	return cf;
}

IniUnit * IniConfig_load(const char * path) {
	if (access(path, F_OK) != 0) {
		return NULL; 
	}
	FILE * fp; 
	char * text = NULL; 
	const char * ptr; 
	long len, readlen; 
	int ret; 
	
	IniUnit * cf = IniConfig_new("default_ini_root"); 
	if ((fp = fopen(path, "r"))) {
		fseek(fp, 0L, SEEK_END); 
		len = ftell(fp); 
		text = (char * )malloc(len + 1); 
		if (text) {
			fseek(fp, 0L, SEEK_SET); 
			readlen = fread(text, 1, len, fp); 
			if (readlen == len) {
				text[len] = '\0'; 
				cf = parse_text(cf, text); 
				if (cf) {
					ret = 0; 
				}else {
					ret = -1; 
				}
			}else ret = -1; 
		
		}else ret = -1; 
	}else ret = -1;
	if( text )
		free(text); 
	if(ret != 0 )
	{
		IniConfig_terminate( cf );
		return NULL;
	}
	return cf; 
}

const char * IniConfig_get_item_safe(IniUnit * cf, const char *key, const char * def_value) {
	StrStrPair * item; 
	PtrList *list =  &cf->items; 
	POSITION    pos; 

	for (pos = list-> head; pos != NULL; pos = pos->next) {
		item = (StrStrPair * )pos->ptr; 
		if (0 == strcmp(key, item->key)) {
			return item->value; 
		}
	}
	return def_value; 
}


#if 0
int main(int argc, char *argv[])
{
	IniUnit *c = IniConfig_load( "test.ini" );
	IniUnit *sub = IniConfig_get_subunit(c, "SYS");
	printf("%s", IniConfig_get_item( sub, "caption" ) );
	printf("%s", IniConfig_get_item( sub, "username" ) );
	
	IniConfig_clean( c );
	return;
}
#endif
