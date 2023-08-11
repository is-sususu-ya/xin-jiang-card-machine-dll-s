

#ifndef _WINTYPE_INCLUDED
#define _WINTYPE_INCLUDED
// Windows wide char and MBCS function set and macro
#define _T(s)		s
#define CONST       const
#define TCHAR	    char
#define _tcschr     strchr
#define _tcsrchr    strrchr
#define _tcsstr     strstr
#define _tcscat		strcat
#define _tcscpy		strcpy
#define _tcsncpy    strncpy
#define _tcscmp     strcmp
#define _tcsncmp    strncmp
#define _tcsdup		strdup
#define _tcslen     strlen
#define _stprintf   sprintf
#define _stnprintf  snprintf
#define _vstpirntf  vsprintf
#define _vsntprintf vsnprintf
#define _ftprintf   fprintf
#define _tcsnccmp   strncmp
#define _tunlink    unlink
#define _trename    rename
#define _tmkdir     mkdir
#define _tfopen     fopen
#define _topen      open
#define _trmdir     rmdir
#define _tstat		stat

#define	IN
#define OUT

#define WAIT_OBJECT_0		0
#define O_BINARY		 	0

#endif

#ifndef _TYPEDEF_WPARAM_
#define _TYPEDEF_WPARAM_
typedef unsigned long	WPARAM;
#endif

#ifndef _TYPEDEF_LPARAM_
#define _TYPEDEF_LPARAM_
typedef unsigned long	LPARAM;
#endif


#ifndef _TYPEDEF_BOOL_
#define _TYPEDEF_BOOL_
typedef int BOOL;
#endif

#ifndef _TYPEDEF_BOOL_L_
#define _TYPEDEF_BOOL_L_
typedef int bool;
#endif
 
#ifndef _TYPEDEF_UINT_
#define _TYPEDEF_UINT_
typedef unsigned int UINT;
#endif

#ifndef _TYPEDEF_SOCKET_
#define _TYPEDEF_SOCKET_
typedef int		SOCKET;
#endif 

#ifndef _TYPEDEF_BYTE_
#define _TYPEDEF_BYTE_
typedef unsigned char BYTE;
#endif
  
#ifndef _TYPDEF_WORD_
#define _TYPDEF_WORD_
typedef unsigned short	WORD;
#endif

#ifndef _TYPDEF_DWORD_
#define _TYPDEF_DWORD_
typedef int		DWORD;	
#endif

#ifndef _TYPEDEF_PVOID_
#define _TYPEDEF_PVOID_
typedef void *	PVOID;
#endif

#ifndef _TYPEDEF_HANDLE_
#define _TYPEDEF_HANDLE_
typedef void * HANDLE;		
#endif

#ifndef _TYPEDEF_LPVOID_
#define _TYPEDEF_LPVOID_
typedef void * LPVOID;		
#endif
 
#ifndef _TYPEDEF_LPCTSTR_
#define _TYPEDEF_LPCTSTR_
typedef const char* LPCTSTR;
#endif

#ifndef _TYPEDEF_LPTSTR_
#define _TYPEDEF_LPTSTR_
typedef char *	LPTSTR;
#endif

#ifndef _TYPEDEF_LPSTR_
#define _TYPEDEF_LPSTR_
typedef  char* LPSTR;
#endif

#ifndef _TYPEDEF_UINT64_
#define _TYPEDEF_UINT64_
typedef unsigned long long UINT64;
#endif
    
#ifndef _TYPEDEF_INT64_
#define _TYPEDEF_INT64_
typedef long long INT64;
#endif


#ifndef _TYPE__INT64_
#define _TYPE__INT64_
typedef long long    __int64;
#endif


 
#ifndef _TYPEDEF_TRUE_FALSE_
#define _TYPEDEF_TRUE_FALSE_
#define FALSE 0
#define TRUE 1
#define true 1
#define false 0
#define FAILURE -1
#define SUCCESS 0
#endif

