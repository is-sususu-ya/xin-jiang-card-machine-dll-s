/*
 * Windows GUID, Linux UUID generator.
 *
 * in linux, needs -luuid, in Windows needs ole32.lib
 *
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef WIN32
#include <ole2.h>
#else
// #include <uuid/uuid.h>
#endif
#include "utils_guid.h"

#define GUID_FORMAT_LOWER		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUID_FORMAT_UPPER		"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"

static const char *guid_format = GUID_FORMAT_UPPER;

unsigned short hexval( char ch )
{
	ch = toupper(ch);
	if ( isdigit(ch) )
		return ch - '0';
	else
		return ch - 'A' + 10;		
}

void guid_set_format(int uppercase)
{
	guid_format =  ( uppercase ) ? GUID_FORMAT_UPPER : GUID_FORMAT_LOWER;
}

void uuid_generate( unsigned char *ulid )
{

}

void uuid_unparse( uuid_t uid, char *str )
{

}

GUID guid_create(void)
{
    GUID guid;
#ifdef WIN32
    CoCreateGuid(&guid);
#else
    uuid_generate( (unsigned char *)(&guid));
#endif
    return guid;
}

static char _guid_str[40] = {0};
const char* guid_to_str(GUID *pguid, char *buf)
{
    char *guid_buf = (buf==NULL) ? _guid_str : buf;
    
#ifdef __GNUC__
    snprintf(
#else // MSVC
    _snprintf_s(
#endif
        guid_buf,
        sizeof(_guid_str),
        guid_format,
        pguid->part_long, pguid->part_short[0], pguid->part_short[1],
        pguid->part_byte[0], pguid->part_byte[1],
        pguid->part_byte[2], pguid->part_byte[3],
        pguid->part_byte[4], pguid->part_byte[5],
        pguid->part_byte[6], pguid->part_byte[7]
     );
	return guid_buf;
}

const char *guid_create_str(char *buf)
{
	GUID guid_tmp = guid_create();
	return guid_to_str(&guid_tmp, buf);
}

GUID guid_from_str(const char *str)
{
	GUID guid;
	char *ptr = (char *)str;
	int i;
	
	guid.part_long = strtol(ptr, &ptr, 16);
	if (*ptr!='-')  goto error_guid;

	for(i=0; i<2; i++ )
	{	
		guid.part_short[i] = strtol(ptr+1, &ptr, 16);
		if (*ptr!='-')  goto error_guid;
	}		

	for(i=0; i<8; i++)
	{
		if ( *ptr=='-' ) ptr++;		
		guid.part_byte[i] = (hexval(ptr[0]) << 4) + hexval(ptr[1]);
		ptr += 2;
	}
	return guid;
	
error_guid:
	memset(	&guid, 0, sizeof(guid) );
	return guid;	
}
