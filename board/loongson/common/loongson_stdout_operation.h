#ifndef __LOONGSON_STDOUT_OPERATION_H__
#define __LOONGSON_STDOUT_OPERATION_H__

#define STDOUT_SERIAL 0
#define STDOUT_VIDEO 1

#define STDOUT_OFF	0
#define STDOUT_ON	1
#define STDOUT_ONLY_ON 2

void set_stdout(int type, int status);

#endif
