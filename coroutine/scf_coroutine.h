#ifndef SCF_COROUTINE_H
#define SCF_COROUTINE_H

#include<sys/epoll.h>
#include<sys/time.h>

#include"scf_list.h"
#include"scf_rbtree.h"

typedef struct scf_co_task_s    scf_co_task_t;
typedef struct scf_co_thread_s  scf_co_thread_t;

struct scf_co_task_s
{
	scf_rbtree_node_t  timer;
	int64_t            time;

	scf_list_t         list;

	scf_co_thread_t*   thread;

	uintptr_t          rip;
	uintptr_t          rsp;
	uintptr_t          rsp0;

	uintptr_t*         stack_data;
	intptr_t           stack_len;
	intptr_t           stack_capacity;

	int                ret;
	uint32_t           events;
};

struct scf_co_thread_s
{
	int                epfd;

	scf_rbtree_t       timers;

	scf_list_t         tasks;
	int                nb_tasks;

	scf_co_task_t*     current;
};

int scf_co_thread_open (scf_co_thread_t** pthread);
int scf_co_thread_close(scf_co_thread_t*  thread);

int scf_co_thread_add_task(scf_co_thread_t* thread, scf_co_task_t* task);

int scf_co_thread_run(scf_co_thread_t*  thread);

#endif

