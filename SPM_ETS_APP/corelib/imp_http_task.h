/* 完成http请求任务 */

#ifndef IMP_HTTP_TASK
#define IMP_HTTP_TASK

typedef void (*fxc_http_response)(int id, int ret, char *response, int size);

void register_http_url(int id, const char *durl);
void http_task_init();
void http_task_exit();
/**
 * @brief 发起请求任务
 * 
 * @param index  第几路post请求地址
 * @param id      请求id
 * @param type     请求类型，json/form-data
 * @param request 
 */
void add_http_task( int index, int id, int type, char *request);
void http_task_setcallback(fxc_http_response cbfx);

#endif