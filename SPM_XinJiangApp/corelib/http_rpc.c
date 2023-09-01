/***************************************************************************
  http_rpc.c  - ????HTTP GET POST???RPC?????????????
	Reworked by: mike dai
	Date:        2022-03-20
	Version:	 1.1.0
	Description:
		using http interface

	  Date: 2022-03-20
	  Version: 1.3.0
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>

#include "utils_str.h"
#include "utils_net.h"
#include "utils_ptrlist.h" 
#include "utils_thread.h"
#include "utils_gbutf8.h"
#include "http_rpc.h"

#define RPC_NONE 0
#define RPC_GET 1
#define RPC_POST 2

#define URL_SIZE 512
#define MAX_BUF_SIZE 4096 // 4KB
#define LARGEBUFFER_SIZE 4096

#define HTTP_SERVICE_PORT 85

typedef struct tagHTTPRequest
{
	int method;
	char url[URL_SIZE];
	int protocol;
	StrMap headers;
} HTTPRequest;

typedef struct tagHTTPContext
{
	pthread_t thread;
	int fd;
	struct sockaddr_in addr;
	HTTPRequest *req;
	char *script_name;	// URI virtual path (point to req->url), ended at '?' in URL
	char *query_string; // variable started (after '?') - NULL if no variables
	StrMap http_vars;	// parsed variables pair (key=value)
} HTTPContext;

typedef int (*HttpHandlerFunc)(HTTPContext *cinfo, const char *param);

typedef struct tagHttpHandler
{
	int method;
	char *prefix;
	int len;
	HttpHandlerFunc func;
	int order;
} HttpHandler;

typedef struct tagrpc_info_s
{
	char url_name[256];
	RPC_HookFucn fxc;
} rpc_info_s;

static pthread_mutex_t HTTPD_lock = PTHREAD_MUTEX_INITIALIZER;
#define httpd_lock() pthread_mutex_lock(&HTTPD_lock)
#define httpd_unlock() pthread_mutex_unlock(&HTTPD_lock)

static pthread_mutex_t rpc_lock = PTHREAD_MUTEX_INITIALIZER;
#define RPC_Lock() pthread_mutex_lock(&rpc_lock)
#define RPC_UnLock() pthread_mutex_unlock(&rpc_lock)
  
static int HTTP_trace = 0; // enable trace info 
static int bQuit = 0;

static PtrList FxcList = PTRLIST_INITIALIZER;
static pthread_t rpc_thread;

//  receive http info
struct resonse_msg
{
	int code;
	const char *msg;
};

struct resonse_msg reply_msg[] = {
	{0, "success"},
	{1, "not support GET method"},
	{2, "not find rpc address"},
	{3, "unknow error"},
	{4, "content-type not a json format"},
	{5, "not receive full request data"},
	{6, "content-length is 0"},
	{7, "not support method"},
	{8, "invalud json syntax"},
	{9, "not support protocol"},
	{10, "not a json request"},
	{11, "not support api"},
};

int http_response_json(int fd, const char *textmsg)
{
	int code = 200;
	char buffer[MAX_BUF_SIZE];
	int len = strlen(textmsg);
	sprintf(buffer, "HTTP/1.0 200 \r\n"
					"Server: rpc service \r\n"
					"Content-Type: application/json \r\n"
					"content-length: %d \r\n"
					"Connection: close\r\n\r\n"
					"%s",
			len, textmsg);
	send(fd, buffer, strlen(buffer), 0);
	printf("RPC RESPON: \r\n %s \r\n", textmsg);
	return code;
}

static const char *get_response_text(int index)
{
	static char error_msg[256];
	int cnt = sizeof(reply_msg) / sizeof(reply_msg[0]);
	index = (index < 0 || index > cnt) ? 0 : index;
	sprintf(error_msg, "{\"code\": %d,\"msg\": \"%s\"}", reply_msg[index].code, reply_msg[index].msg);
	return error_msg;
}

static int http_cmp(const void *ptr1, const void *ptr2)
{
	const HttpHandler *hndl1 = (HttpHandler *)ptr1;
	const HttpHandler *hndl2 = (HttpHandler *)ptr2;
	return hndl1->order - hndl2->order;
}

static PtrList HTTPD_handlers = {0, NULL, NULL, http_cmp};

static int HTTPD_init_context(HTTPContext *cinfo);

static int httpd_register(int method, const char *prefix, HttpHandlerFunc func, int order)
{
	HttpHandler *handler;

	if (!prefix || prefix[0] == '\0')
		return -1; // no prefix.
	handler = malloc(sizeof(HttpHandler));
	if (!handler)
		return -ENOMEM;
	handler->method = method;
	handler->prefix = strdup(prefix);
	handler->len = strlen(prefix);
	handler->func = func;
	handler->order = order;
	httpd_lock();
	PtrList_insert_1to9(&HTTPD_handlers, handler);
	httpd_unlock();
	return 0;
}

int HTTPD_register(int method, const char *prefix, HttpHandlerFunc func)
{
	return httpd_register(method, prefix, func, 0);
}

int HTTPD_register_ex(int method, const char *prefix, HttpHandlerFunc func, int order)
{
	return httpd_register(method, prefix, func, order);
}

int HTTPD_unregister(HttpHandlerFunc func)
{
	HttpHandler *handler;
	PtrList *list = &HTTPD_handlers;
	POSITION pos, posdel;

	httpd_lock();
	for (pos = list->head; pos != NULL;)
	{
		handler = (HttpHandler *)pos->ptr;
		posdel = pos;
		pos = pos->next;
		if (func == handler->func)
		{
			handler = PtrList_remove(list, posdel);
			if (handler->prefix)
				free(handler->prefix);
			free(handler);
		}
	}
	httpd_unlock();
	return 0;
}

static void httpd_destroy_task(HTTPContext *cinfo)
{
	if (cinfo->req)
	{
		StrMap_remove_all(&cinfo->req->headers);
		free(cinfo->req);
		cinfo->req = NULL;
	}
	if (cinfo->script_name)
	{
		cinfo->script_name = NULL;
		StrMap_remove_all(&cinfo->http_vars);
	}

	if (cinfo->fd)
	{
		close(cinfo->fd);
		cinfo->fd = 0;
	}
	free(cinfo);
}

static int rpc_receive_headers(int fd, StrMap *strmap)
{
	int n = 0;
	char buffer[LARGEBUFFER_SIZE], key[PATH_MAX], value[PATH_MAX];
	const char *ptr, *q;
	int len;

	PtrList *list = strmap;
	StrStrPair *header;

	/* go on to parse headers */
	for (n = 0; n < 100; n++)
	{ /* we assume 100 as the max number */

		len = sock_read_line(fd, buffer, LARGEBUFFER_SIZE);
		if (len <= 0)
		{ /* bad request, interrupted by client? */
			StrMap_remove_all(list);
			return -1;
		}
		buffer[len] = '\0';

		/* we meet a blank line --- end line of request,
		 * we return with headers */
		if (buffer[0] == '\n' || (buffer[0] == '\r' && buffer[1] == '\n'))
			break;

		ptr = buffer;

		/* parse the first word as key */
		for (q = ptr; *q != ':' && *q != '\0'; q++)
			;
		len = q - ptr;
		if (len >= PATH_MAX)
			continue;
		memcpy(key, ptr, len);
		key[len] = '\0';

		for (ptr = q; *ptr == ':' || *ptr == ' ' || *ptr == '\t'; ptr++)
			;

		/* parse the other till end of line as value */
		for (q = ptr; *q != '\r' && *q != '\n' && *q != '\0'; q++)
			;
		len = q - ptr;
		if (len >= PATH_MAX)
			continue;
		memcpy(value, ptr, len);
		value[len] = '\0';

#ifdef DEBUGREQ
		fprintf(stderr, "%s: %s\n", verb, url);
#endif
		/* we got a new header, add to the list */
		header = malloc(sizeof(StrStrPair));
		header->key = strdup(key);
		header->value = strdup(value);
		PtrList_append(list, header);
	} 
	return n;
}
  
static const char *rpc_method_string(int method)
{
	switch (method)
	{
	case RPC_NONE:
		return "NONE";
		break;
	case RPC_GET:
		return "GET";
		break;
	case RPC_POST:
		return "POST";
		break;
	default:
		return "NONE";
	}
}

static int rpc_method_init(const char *set)
{
	if (strcmp(set, "GET") == 0)
		return RPC_GET;
	if (strcmp(set, "POST") == 0)
		return RPC_POST;
	return RPC_NONE;
}

static HTTPRequest *HTTPD_parse_request(int fd)
{
	char buffer[MAX_BUF_SIZE], strMethod[PATH_MAX];
	int len;
	char *ptr, *q;
	HTTPRequest *req;
	len = sock_read_line(fd, buffer, MAX_BUF_SIZE);
	if (len <= 0)
		return NULL;
	buffer[len] = '\0';
	req = malloc(sizeof(HTTPRequest));
	if (req)
	{
		memset(req, 0, sizeof(HTTPRequest));
		if (strgetword(buffer, strMethod, PATH_MAX, &q) &&
			strgetword(q, req->url, URL_SIZE, &q))
		{
			req->method = rpc_method_init(strMethod);
			for (ptr = q; *ptr == ' ' || *ptr == '\t'; ptr++)
				;
			if (0 == memcmp(ptr, "HTTP/", 5))
			{
				ptr += 5;
				req->protocol = strtod(ptr, NULL) * 10;
				rpc_receive_headers(fd, &req->headers);
			}
			return req;
		}
		else
			return NULL;
	}
	else
		return NULL;
}

static void *HTTPD_working_thread_func(void *arg)
{
	HTTPContext *cinfo = arg;
	HTTPRequest *req;
	PtrList *list; 
	POSITION pos;
	HttpHandler *handler;
	int http_code;
	cinfo->thread = pthread_self();
	req = cinfo->req = HTTPD_parse_request(cinfo->fd);
	http_code = 0;
	if (cinfo->req != NULL)
	{
		if (HTTP_trace)
		{
			printf("%s:%d => %s %s HTTP/%d.%d\r\n", inet_ntoa(cinfo->addr.sin_addr), ntohs(cinfo->addr.sin_port),
				   rpc_method_string(req->method), req->url, req->protocol / 10, req->protocol % 10);
			StrStrPair *header;
			printf("\n%s:%d => HTTP {\n", inet_ntoa(cinfo->addr.sin_addr), ntohs(cinfo->addr.sin_port));
			printf("%s %s HTTP/%d.%d\n", rpc_method_string(req->method), req->url, req->protocol / 10, req->protocol % 10);
			list = &req->headers;
			for (pos = list->head; pos != NULL; pos = pos->next)
			{
				header = (StrStrPair *)PtrNode_get(pos);
				printf("%s: %s\n", header->key, header->value);
			}
		}
		HTTPD_init_context(cinfo);
		list = &HTTPD_handlers;
		for (pos = list->head; pos != NULL; pos = pos->next)
		{
			handler = (HttpHandler *)pos->ptr;
			if (!strncasecmp(req->url, handler->prefix, handler->len) &&
				((RPC_NONE == handler->method) || (req->method == handler->method)))
			{
				http_code = 200;
				handler->func(cinfo, req->url + strlen(handler->prefix));
				break;
			}
		}
		if (!http_code)
		{
			http_response_json(cinfo->fd, get_response_text(2));
			httpd_destroy_task(cinfo);
			return NULL;
		}
	}
	else
	{
		printf("parse http request failed! .\r\n");
	}
	httpd_destroy_task(cinfo);
	return NULL;
}

static int rpc_url_decode(const char *str, int len, char *buf, int bufsize)
{
	const char *src, *src_end;
	char *dst, *eostr, hex[3];
	src = str;
	src_end = src + len;
	for (dst = buf, eostr = buf + bufsize - 1; src < src_end; dst++)
	{
		if (dst >= eostr)
			return -1;
		switch (*src)
		{
		case '%':
			src++;
			hex[0] = *src++;
			hex[1] = *src++;
			hex[2] = 0;
			*dst = (strtol((char *)hex, NULL, 16) & 0xFF);
			break;
		default:
			*dst = *src;
			src++;
			break;
		}
	}
	*dst = '\0';
	return dst - buf;
}
 
static int http_parse_string_tolist(const char *text, StrMap *strmap)
{
	int len = strlen(text);
	const char *text_end = text + len;
	const char *str, *ptr, *q = NULL;
	char key[PATH_MAX];
	char value[URL_SIZE];

	str = ptr = text;
	while (*str != '\0')
	{
		if ((q = strchr(str, '&')) == NULL)
			q = text_end;
		else if (*(q + 1) == '\0') // extra '&' on end of body
			break;
		if ((ptr = strchr(str, '=')) != NULL && ptr < q)
		{
			rpc_url_decode(str, ptr - str, key, PATH_MAX);
			rpc_url_decode(ptr + 1, q - ptr - 1, value, URL_SIZE);
		}
		else
		{
			rpc_url_decode(str, q - str, key, PATH_MAX);
			value[0] = '\0';
		}
		printf("parse vars: key=%s, value=%s\n", key, value);
		StrMap_set(strmap, key, value);
		if (*q == '&')
			q++;
		str = ptr = q;
	}
	return PtrList_get_count(strmap);
}

static int HTTPD_init_context(HTTPContext *cinfo)
{
	char *str;
	if (cinfo->script_name != NULL)
		return 0;
	cinfo->script_name = cinfo->req->url;
	str = strchr(cinfo->req->url, '?');
	if (str != NULL)
	{
		*str++ = '\0';
		cinfo->query_string = str;
		http_parse_string_tolist(str, &cinfo->http_vars);
	}
	return 0;
}

static int rpc_onrequest(int fd)
{
	int new_fd;
	socklen_t len;
	struct sockaddr_in from_addr;
	HTTPContext *cnew;
#ifdef ENBALE_THREADPOOL
	pthread_t threadId;
#endif
	len = sizeof(from_addr);
	new_fd = accept(fd, (struct sockaddr *)&from_addr, &len);
	if (new_fd <= 0)
		return -1;
	cnew = malloc(sizeof(HTTPContext));
	if (cnew)
	{
		httpd_lock();
		memset(cnew, 0, sizeof(HTTPContext));
		cnew->fd = new_fd;
		memcpy(&cnew->addr, &from_addr, sizeof(struct sockaddr_in));
#ifdef ENBALE_THREADPOOL
		THREAD_create(&threadId, NULL, HTTPD_working_thread_func, cnew, "HTTPD service thread");
#else
		HTTPD_working_thread_func(cnew);
#endif
		httpd_unlock();
	}
	return 0;
}

static int read_http_request(HTTPContext *cinfo, char *buffer, int size)
{
	int len = 0; 
	char gbk_buf[MAX_BUF_SIZE] = {0};
	long content_length = 0;
	const char *content_type = NULL;
	if (cinfo->req->method == RPC_GET)
	{
		return http_response_json(cinfo->fd, get_response_text(1));
	}
	else if (cinfo->req->method == RPC_POST)
	{
		content_length = strtol(StrMap_safe_get(&cinfo->req->headers, "Content-Length", "0"), NULL, 0);
		content_type = StrMap_safe_get(&cinfo->req->headers, "Content-Type", "");
		if (!strstr(content_type, "json"))
			return http_response_json(cinfo->fd, get_response_text(4));
		if (content_length > 0 && content_length < MAX_BUF_SIZE)
		{
			len = sock_read_n_bytes_tout(cinfo->fd, buffer, content_length, 100);
			if (len == content_length)
			{
				if (buffer[0] != '{')
					return http_response_json(cinfo->fd, get_response_text(8));
				if (strstr(content_type, "UTF") || strstr(content_type, "utf"))
				{
					UTF8ToGBK(buffer, strlen(buffer), gbk_buf);
					memset(buffer, 0, MAX_BUF_SIZE);
					strcpy(buffer, gbk_buf);
				}
				return 0;
			}
			else
			{
				printf("FD[%d] receive len:%d need:%d \r\n", cinfo->fd, len, (int)content_length);
				return http_response_json(cinfo->fd, get_response_text(5));
			}
		}
		else
		{
			printf("http content length too short or too long!\r\n");
			return http_response_json(cinfo->fd, get_response_text(6));
		}
	}
	else
	{
		return http_response_json(cinfo->fd, get_response_text(7));
	}
}

static int http_api_proc(HTTPContext *cinfo, const char *param)
{ 
	POSITION pos;
	int ret = 0;
	char buffer[MAX_BUF_SIZE] = {0};
	char outbuf[MAX_BUF_SIZE] = {0};
	if (read_http_request(cinfo, buffer, sizeof(buffer)) != 0)
	{
		printf("http request data parse failed \r\n");
		return 0;
	}
	for (pos = FxcList.head; pos != NULL; pos = pos->next)
	{
		rpc_info_s *node = (rpc_info_s *)pos->ptr;
		if (strstr(cinfo->req->url, node->url_name) && node->fxc)
		{
			ret = node->fxc(buffer, outbuf);
			if (outbuf[0] == '{')
				return http_response_json(cinfo->fd, outbuf);
			sprintf(buffer, "{\"code\": %d,\"msg\": \"%s\"}", ret, outbuf);
			return http_response_json(cinfo->fd, buffer);
		}
	}
	return http_response_json(cinfo->fd, get_response_text(2));
}

static void *rpc_work_thread(void *arg)
{ 
	int listen_tcp = sock_listen(HTTP_SERVICE_PORT, NULL, 5);
	while (!bQuit)
	{
		struct timeval tv;
		fd_set read_fds;
		int nsel;
		FD_ZERO(&read_fds);
		FD_SET(listen_tcp, &read_fds);
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		nsel = select(listen_tcp + 1, &read_fds, NULL, NULL, &tv);
		if (nsel > 0)
		{
			if (FD_ISSET(listen_tcp, &read_fds))
			{
				rpc_onrequest(listen_tcp);
			}
		}
	}
	return NULL;
}

int RPC_startup()
{
	printf("rpc start!\r\n");
	pthread_create(&rpc_thread, NULL, rpc_work_thread, NULL);
	return 0;
}

int RPC_shutdown()
{
	bQuit = 1;
	pthread_cancel(rpc_thread);
	pthread_join(rpc_thread, NULL);
	RPC_Lock();
	PtrList_delete_all(&FxcList);
	RPC_UnLock();
	return 0;
}

int RPC_register_call(const char *url, RPC_HookFucn fxc)
{
	if (url[0] != '/')
		return -1;
	rpc_info_s *node = malloc(sizeof(rpc_info_s));
	strcpy(node->url_name, url);
	node->fxc = fxc;
	RPC_Lock();
	PtrList_append(&FxcList, node);
	RPC_UnLock();
	HTTPD_register_ex(RPC_POST, url, http_api_proc, 1);
	return 0;
}

#ifdef ENABLE_TEST_CODE
int main(int argc, char const *argv[])
{
	/* code */
	RPC_startup();
	return 0;
}

#endif