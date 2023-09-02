#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "http_rpc.h"
#include "cJSON.h"
#include "utils_ptrlist.h"

static cJSON *list_to_json_object(StrMap *list)
{
	cJSON *root = cJSON_CreateObject();
	POSITION pos = list->head;
	for (; pos != NULL; pos = pos->next)
	{
		StrStrPair *pair = pos->ptr;
		if (pair->value)
			cJSON_AddStringToObject(root, pair->key, pair->value);
	}
	return root;
}

static const char *list_to_json_string(StrMap *list)
{
	cJSON *root = list_to_json_object(list);
	const char *str;
	str = cJSON_Print(root);
	cJSON_Delete(root);
	return str;
}

static const char *int32_to_string(int32_t val)
{
	static char buf[128] = {0};
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", val);
	return buf;
}

static const char *uint32_to_string(int32_t val)
{
	static char buf[128] = {0};
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%ud", val);
	return buf;
}

static int json_to_list(cJSON *node, StrMap *list)
{
	int count = 0;
	cJSON *next;
	if (!node || node->type != cJSON_Object)
		return 0;
	next = node->child;
	while (next)
	{
		count++;
		if (next->type == cJSON_String)
			StrMap_set(list, next->string, next->valuestring);
		else if (next->type == cJSON_Number)
			StrMap_set(list, next->string, int32_to_string(next->valueint));
		else if (next->type == cJSON_False)
			StrMap_set(list, next->string, "false");
		else if (next->type == cJSON_True)
			StrMap_set(list, next->string, "yes");
		else
		{
			// not string or number ignore it, maybe it is a object, a null or array,we cannot handle it
			count--;
		}
		next = next->next;
	}
	return count;
}

static int string_to_list(const char *input, StrMap *list)
{
	cJSON *root = cJSON_Parse(input);
	if (!root)
		return -1;
	json_to_list(root, list);
	cJSON_Delete(root);
	return 0;
}

extern void trace_log(const char *fmt, ...);

static int rpc_qrcode_fake(const char *input, char *output)
{
	const char *phoneId = NULL;
	StrMap maplist = PTRLIST_INITIALIZER;
	string_to_list(input, &maplist);
	phoneId = StrMap_safe_get(&maplist, "phoneId", "");
	if (strcmp(phoneId,"") == 0)
	{
		trace_log("no phoneId parameter..");
		strcpy(output, "no phoneId");
		return 0;
	}
	else
	{
		trace_log("phoneId :%s", phoneId);
		strcpy(output, "success");
		spm_answer_talk(phoneId);
		return 0;
	}
}

int test_interface()
{
	RPC_register_call("/calling", rpc_qrcode_fake);
}
