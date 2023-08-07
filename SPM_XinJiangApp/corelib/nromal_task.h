
#ifndef NORMAL_TASK_HEADER_H
#define NORMAL_TASK_HEADER_H

struct task_content
{
    int param[32];
    char text[256];
};

#define TASK_LED_SHOW        1
#define TASK_LED_CTR         2
#define TASK_SOUND_PLAY      3
#define TASK_SYSTEM_ERROR    10

void task_init();
void task_exit();
void task_add( int type, int param, void *msg, int size );

#endif