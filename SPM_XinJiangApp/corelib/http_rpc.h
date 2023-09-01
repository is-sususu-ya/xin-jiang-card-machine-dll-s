/***************************************************************************
                          core_httpd.h  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HTTPD_CORE_H_
#define _HTTPD_CORE_H_

#include <pthread.h>
#include <netinet/in.h>
#include <utils_ptrlist.h>
 
typedef int (*RPC_HookFucn)(const char *inParm, char *outParma);

extern int RPC_startup();
extern int RPC_shutdown();
int RPC_register_call(const char *url, RPC_HookFucn fxc );
 
#endif /*  _HTTPD_CORE_H_ */



