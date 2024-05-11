#include <common.h>
#include <stdlib.h>
#include <command.h>

#include "loongson_stdout_operation.h"

static void set_stdout_off(char* stdout_dev)
{
	char* env_old;
	char* temp;
	char* new_env;
	int all_len;
	int len_front;
	int len_back;
	int len_param;

	if (!stdout_dev)
		return;

	env_old = env_get("stdout");
	if (!env_old)
		return;

	temp = strstr(env_old, stdout_dev);
	if (!temp)
		return;

	all_len = strlen(env_old);
	len_front = temp - env_old;
	len_param = strlen(stdout_dev);
	len_back = all_len - len_front - len_param;

	new_env = (char*)calloc(all_len + 1, sizeof(char));
	memcpy(new_env, env_old, len_front);
	if (len_back)
		strcpy(new_env + len_front, temp + len_param + 1);

	all_len = len_front + len_back;

	if (new_env[all_len - 1] == ',')
		new_env[all_len - 1] = 0;

	env_set("stdout", new_env);
	free(new_env);
}

static void set_stdout_on(char* stdout_dev)
{
	char* env_old;
	char* temp;
	char* new_env;
	int len;

	if (!stdout_dev)
		return;

	env_old = env_get("stdout");

	if (!env_old)
		env_set("stdout", stdout_dev);

	temp = strstr(env_old, stdout_dev);
	if (temp)
		return;

	len = strlen(env_old) + strlen(stdout_dev) + 4;
	new_env = (char*)calloc(len, sizeof(char));
	sprintf(new_env,"%s,%s", env_old, stdout_dev);
	env_set("stdout", new_env);
	free(new_env);
	return;
}

static void set_stdout_only_on(char* stdout_dev)
{
	if (!stdout_dev)
		return;

	env_set("stdout", stdout_dev);
}

typedef void (*set_stdout_func)(char*);

void set_stdout(int type, int status)
{
	char serial_env[] = "serial";
#ifdef CONFIG_VIDEO
	char video_env[] = "vidconsole,vidconsole1";
	char* handle_env_set[] = {serial_env, video_env};
#else
	char* handle_env_set[] = {serial_env};
#endif
	char* handle_env;
	set_stdout_func set_stdout_func_set[] = {set_stdout_off, set_stdout_on, set_stdout_only_on};

	if (type < STDOUT_SERIAL || type > STDOUT_VIDEO)
		return;
	if (status < STDOUT_OFF || status > STDOUT_ONLY_ON)
		return;

	handle_env = handle_env_set[type];
	(*set_stdout_func_set[status])(handle_env);
}
