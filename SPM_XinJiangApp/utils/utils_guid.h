#ifndef _UTILS_GUID_INCLUDED_
#define _UTILS_GUID_INCLUDED_


typedef int uuid_t;
typedef struct tagGUID {
	unsigned long  part_long;
	unsigned short part_short[2];
	unsigned char part_byte[8];
} GUID;

/*
 * note - in linux, uuid_t is defined as 
 * typedef unsigned char uuid_t[16]; (in <uuid/uuid.h>
 */
 
GUID guid_create(void);
void uuid_unparse( uuid_t uid, char *str );
void uuid_generate( unsigned char *ulid );
const char *guid_create_str(char *buf);
const char *guid_to_str(GUID *pguid, char *buf);
GUID guid_from_str(const char *str);
void guid_set_format(int uppercase);		// default is upper case like "72af572d-8487-4cd1-bb30-cc48dd0d2351"

#endif
