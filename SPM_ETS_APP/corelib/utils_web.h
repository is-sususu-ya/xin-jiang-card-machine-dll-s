 

#ifndef _HEADER_UTILS_WEB_
#define _HEADER_UTILS_WEB_

/**
 * @brief 所有函数，返回0时表示正确，返回非0时标识错误码
 * 
 */

#define NET_ERROR_NONE 0
#define NET_ERROR_URL_INVALID -1
#define NET_ERROR_CANNOT_CONNECTION_TO_SERVER -2
#define NET_ERROR_SERVER_REPLY_NOT_OK -3
#define NET_ERROR_SERVER_REPLY_NOT_KV -4
#define NET_ERROR_SELECT_POLL_ERROR  -5
#define NET_ERROR_PARAM         -10

/**
 * @brief 返回码对应的错误描述信息
 * 
 * @param code 
 * @return const char* 
 */
const char *ErrorString(int code); 
/**
 * @brief 设置http的读写超时时间
 * 
 * @param connect_tout 
 * @param process_tout 
 * @return int 
 */
int http_set_timeout( int connect_tout, int process_tout );

int http_get(const char *url, char *response, int max_size);

/**
 * @brief 通用http请求，支持http,https，请求类型支持 form-data、json格式，请求数据请使用UTF8编码
 * 
 * @param url 请求地址，http或者https打头，标准url地址
 * @param type 请求类型， 0：json, 1:form-data
 * @param reqeust 请求参数，body数据，用户自行准备
 * @param response 响应参数
 * @param max_size 响应buf的最大长度
 * @return int 错误值返回码
 */
int http_post(const char *url, int type, const char *reqeust, char *response, int max_size);
/**
 * @brief 下载文件
 * 
 * @param url 下载地址
 * @param fileName  文件保存的路径名
 * @return int 错误值返回码
 */
int http_download_file(const char *url, const char *fileName);
/**
 * @brief 上传文件
 * 
 * @param url post地址
 * @param fileName 文件路径，目前只支持form-data类型
 * @return int 错误值返回码
 */
int http_upload_file(const char *url, const char *fileName);
 
#endif

 