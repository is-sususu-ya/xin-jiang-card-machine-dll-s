 
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <utils_gbutf8.h>
#include <netdb.h>
#include <curl/curl.h>
#include "utils_web.h"
#include "dbg_printf.h"

#define min(a, b) ((a) > (b) ? (b) : (a))

typedef struct tagDataBuffer
{
	int size;
	char *buffer;
} CurlCallBackDataBuffer;
  
static int CONNECT_TIMEOUT = 5;
static int PROCESS_TIMEOUT = 10;

#define USING_GLOBAL_BUFFER
 
#ifdef USING_GLOBAL_BUFFER
static char *http_response_buffer = NULL;

static void buf_init()
{
	if( http_response_buffer == NULL)
		http_response_buffer = malloc( 1024*1024 );
}
#endif
 
const char *ErrorString(int code)
{
	if (code == NET_ERROR_NONE)
		return "No Error..\r\n";
	else if (code == NET_ERROR_URL_INVALID)
		return "ULR Invalid.";
	else if (code == NET_ERROR_CANNOT_CONNECTION_TO_SERVER)
		return "connect to server error.";
	else if (code == NET_ERROR_SERVER_REPLY_NOT_OK)
		return "server not reply 200 ok.";
	else if (code == NET_ERROR_SERVER_REPLY_NOT_KV)
		return "server reply not a key value pair";
	else if (code == NET_ERROR_SELECT_POLL_ERROR)
		return "server error happend when select fd";
	else
		return "not defined error happend!";
}

static size_t http_post_respose(void *ptr, size_t size, size_t nmemb, void *usrData)
{ 
#ifdef USING_GLOBAL_BUFFER
	CurlCallBackDataBuffer *node = (CurlCallBackDataBuffer *)usrData;
	int data_offset = node->size; 
	memcpy(http_response_buffer + data_offset, ptr, size * nmemb);
	node->size = node->size + size * nmemb;
	http_response_buffer[node->size] = 0;
	return size * nmemb;
#else
	CurlCallBackDataBuffer *node = (CurlCallBackDataBuffer *)usrData;
	int data_offset = node->size;
	node->size += size * nmemb;
	node->buffer = (char *)realloc(node->buffer, node->size); 
	memcpy(node->buffer + data_offset, ptr, size * nmemb);
	return size * nmemb;
#endif
}

static size_t write_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static void CurlBufferRelease(CurlCallBackDataBuffer *curl_buf)
{
	if (curl_buf->buffer)
		free(curl_buf->buffer);
}

static int real_http_post(const char *url,  int type, const char *request, char *response, int max_size)
{ 
	char errbuf[CURL_ERROR_SIZE] = {0};
	struct curl_slist *headers = NULL;
	CURL *hnd = NULL;
	int code = NET_ERROR_NONE;
	int enable_ssl = 1L;
	int enable_ca = 1L; 
	if( strncmp(url, "https", 5 ) != 0 )
	{
		enable_ssl = 0;
		enable_ca = 0;
	} 
#ifdef USING_GLOBAL_BUFFER
	buf_init();
#endif
	CurlCallBackDataBuffer curl_buf;
	memset(&curl_buf, 0, sizeof(curl_buf)); 
	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, errbuf); 
	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
	if (enable_ssl && enable_ca)
	{
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);	   // 只信任CA颁布的证书
		curl_easy_setopt(hnd, CURLOPT_CAINFO, "./certs/cacert.pem"); // CA根证书（用来验证的网站证书是否是CA颁布）
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 2);	    // 检查证书中是否设置域名，并且是否与提供的主机名匹配
	}
	else if (enable_ssl)
	{
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L); // 信任任何证书
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 1L); // 检查证书中是否设置域名
	}
	curl_easy_setopt(hnd, CURLOPT_URL, url);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, http_post_respose);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&curl_buf);
	headers = curl_slist_append(headers, "cache-control: no-cache");
	if( type == 0 )
	{
		headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");	
		headers = curl_slist_append(headers, "Accept: application/json"); 
		headers = curl_slist_append(headers, "Accept-Charset: UTF-8");  
	}
	else
	{
		// 请求的时候是form-data格式，但是要求响应的时候以json格式响应
		headers = curl_slist_append(headers, "content-type: application/x-www-form-urlencoded");
		headers = curl_slist_append(headers, "Accept: application/json"); 
		headers = curl_slist_append(headers, "Accept-Charset: UTF-8");  
	} 
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers); 
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, request);
	curl_easy_setopt(hnd, CURLOPT_TIMEOUT, PROCESS_TIMEOUT);			//设置处理超时
	curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT); 	// 设置链接超时
	CURLcode ret = curl_easy_perform(hnd);
	if (ret == CURLE_OK)
	{ 
#ifdef USING_GLOBAL_BUFFER
		int size = min(max_size, curl_buf.size); 
		strncpy( response, http_response_buffer, size );
#else
		int size = min(max_size, curl_buf.size); 
		strncpy( response, curl_buf.buffer, size );
#endif
	}
	else if (CURLE_OPERATION_TIMEDOUT == ret || CURLE_COULDNT_CONNECT == ret)
	{
		PRINTF("网络访问超时，请检查网络是否断开...\r\n");
		code = NET_ERROR_CANNOT_CONNECTION_TO_SERVER;
	}
	else if (ret == CURLE_SEND_ERROR || ret == CURLE_RECV_ERROR)
	{
		PRINTF("select poll 发生错误！\r\n");
		code = NET_ERROR_SELECT_POLL_ERROR;
	}
	else
	{
		PRINTF("后台返回非200回复，认定回复内容错误！\r\n %s \r\n", errbuf);
		PRINTF("返回码:%d \r\n", ret);
		code = NET_ERROR_SERVER_REPLY_NOT_OK;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(hnd);
	CurlBufferRelease(&curl_buf);
	return code;
}


int http_uplaod_file(const char *url, const char *file_path)
{
	int ret = 0;
	CURL *curl;
	CURLcode res;
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	char file_name[256];
	const char *ptr = file_path;
	int len = strlen(ptr);
	char errbuf[CURL_ERROR_SIZE] = {0};
	int enable_ssl = 1L;
	int enable_ca = 1L; 
	if( strncmp(url, "https", 5 ) != 0 )
	{
		enable_ssl = 0;
		enable_ca = 0;
	}
	for (ptr = file_path + len; *ptr != '/' && ptr > file_path; ptr++)
		;
	strcpy(file_name, ptr + 1);
	if (file_name[0] == '\0')
	{
		PRINTF("无法解析文件名...\r\n");
		return -1;
	}
	curl_formadd(&formpost, &lastptr,CURLFORM_COPYNAME, "file", 	CURLFORM_FILE, file_path, CURLFORM_END);
	curl_formadd(&formpost, &lastptr,CURLFORM_COPYNAME, "fileName",	CURLFORM_COPYCONTENTS, file_name,CURLFORM_END);
	curl = curl_easy_init();
	if (enable_ssl && enable_ca)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);	   // 只信任CA颁布的证书
		curl_easy_setopt(curl, CURLOPT_CAINFO, "./certs/cacert.pem"); // CA根证书（用来验证的网站证书是否是CA颁布）
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);	    // 检查证书中是否设置域名，并且是否与提供的主机名匹配
	}
	else if (enable_ssl)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 信任任何证书
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L); // 检查证书中是否设置域名
	}
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, PROCESS_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		PRINTF("imagefile_upload failed. [ %s ]\r\n", errbuf);
		ret = -2;
	}
	else
	{
		PRINTF("imagefile_upload success. [ %s ]\r\n", errbuf);
	}
	curl_easy_cleanup(curl);
	curl_formfree(formpost);
	return ret;
}

int http_download_file(const char *url, const char *fileName)
{
	CURL *curl_handle = NULL;
	FILE *file = NULL;
	int ret = 0;
	char errbuf[CURL_ERROR_SIZE] = {0}; 
	int enable_ssl = 1L;
	int enable_ca = 1L; 
	if( strncmp(url, "https", 5 ) != 0 )
	{
		enable_ssl = 0;
		enable_ca = 0;
	}
	curl_handle = curl_easy_init();
	if (enable_ssl && enable_ca)
	{
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);	   // 只信任CA颁布的证书
		curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "./certs/cacert.pem"); // CA根证书（用来验证的网站证书是否是CA颁布）
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2);	    // 检查证书中是否设置域名，并且是否与提供的主机名匹配
	}
	else if (enable_ssl)
	{
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L); // 信任任何证书
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 1L); // 检查证书中是否设置域名
	}
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file);
	file = fopen(fileName, "wb+");
	if (file)
	{
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);
		CURLcode res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK)
		{
			PRINTF("down load file failed. [ %s ]\r\n", errbuf);
			ret = -2;
		}
		else
		{
			PRINTF("down load file to [%s] success.\r\n", fileName);
		}
		fclose(file);
	}
	else
	{
		PRINTF("open file:%s error,%s \r\n", fileName, strerror(errno));
	}
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	return 0;
} 
 
int http_get(const char *url, char *response, int max_size)
{
	char errbuf[CURL_ERROR_SIZE] = {0};
	struct curl_slist *headers = NULL;
	CURL *hnd = NULL;
	int code = NET_ERROR_NONE;
	int enable_ssl = 1L;
	int enable_ca = 1L; 
	if( strncmp(url, "https", 5 ) != 0 )
	{
		enable_ssl = 0;
		enable_ca = 0;
	}
#ifdef USING_GLOBAL_BUFFER
	buf_init();
#endif
	CurlCallBackDataBuffer curl_buf;
	memset(&curl_buf, 0, sizeof(curl_buf)); 
	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, errbuf); 
	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	if (enable_ssl && enable_ca)
	{
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);	   // 只信任CA颁布的证书
		curl_easy_setopt(hnd, CURLOPT_CAINFO, "./certs/cacert.pem"); // CA根证书（用来验证的网站证书是否是CA颁布）
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 2);	    // 检查证书中是否设置域名，并且是否与提供的主机名匹配
	}
	else if (enable_ssl)
	{
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L); // 信任任何证书
		curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 1L); // 检查证书中是否设置域名
	}
	curl_easy_setopt(hnd, CURLOPT_URL, url);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, http_post_respose);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&curl_buf);
	headers = curl_slist_append(headers, "cache-control: no-cache"); 
	headers = curl_slist_append(headers, "Accept: application/json");  
	headers = curl_slist_append(headers, "Accept-Charset: UTF-8");    
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);  
	curl_easy_setopt(hnd, CURLOPT_TIMEOUT, PROCESS_TIMEOUT);			//设置处理超时
	curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT); 	// 设置链接超时
	CURLcode ret = curl_easy_perform(hnd);
	if (ret == CURLE_OK)
	{
#ifdef USING_GLOBAL_BUFFER
		int size = min(max_size, curl_buf.size); 
		strncpy( response, http_response_buffer, size );
#else
		int size = min(max_size, curl_buf.size); 
		strncpy( response, curl_buf.buffer, size );
#endif
	}
	else if (CURLE_OPERATION_TIMEDOUT == ret || CURLE_COULDNT_CONNECT == ret)
	{
		PRINTF("网络访问超时，请检查网络是否断开...\r\n");
		code = NET_ERROR_CANNOT_CONNECTION_TO_SERVER;
	}
	else if (ret == CURLE_SEND_ERROR || ret == CURLE_RECV_ERROR)
	{
		PRINTF("select poll 发生错误！\r\n");
		code = NET_ERROR_SELECT_POLL_ERROR;
	}
	else
	{
		PRINTF("后台返回非200回复，认定回复内容错误！\r\n %s \r\n", errbuf);
		PRINTF("返回码:%d \r\n", ret);
		code = NET_ERROR_SERVER_REPLY_NOT_OK;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(hnd);
	CurlBufferRelease(&curl_buf);
	return code;
}
 
int http_post(const char *url, int type, const char *request, char *response, int max_size)
{
	if( type < 0 || type > 1 || request == NULL || response == NULL || max_size == 0 )
		return NET_ERROR_PARAM;
	if( strncmp(url, "http", 4 ) != 0 )
	{
		PRINTF("URL INVLAID: %s \r\n", url );
		return NET_ERROR_URL_INVALID;
	}
	return real_http_post( url, type, request,  response, max_size ); 
} 

int http_set_timeout(int connect_tout, int process_tout)
{
	CONNECT_TIMEOUT = connect_tout > 0 ? connect_tout : 2;
	PROCESS_TIMEOUT = process_tout > 0 ? process_tout : 2;
	PRINTF("连接超时设置为：%d s处理超时设置为：%d s \r\n", CONNECT_TIMEOUT, PROCESS_TIMEOUT);
}
   