#ifndef SCF_COROUTINE_H
#define SCF_COROUTINE_H

#include"scf_list.h"
#include"scf_rbtree.h"

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<sys/epoll.h>
#include<sys/time.h>
#include<fcntl.h>

typedef struct scf_co_task_s    scf_co_task_t;
typedef struct scf_co_thread_s  scf_co_thread_t;

typedef struct scf_co_buf_s     scf_co_buf_t;

struct scf_co_buf_s
{
	scf_co_buf_t*      next;

	size_t             len;
	size_t             pos;
	uint8_t            data[0];
};

#define SCF_CO_ERROR   -1
#define SCF_CO_OK       0
#define SCF_CO_CONTINUE 1

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

	int                err;

	uint32_t           events;
	int                fd;

	scf_co_buf_t*      read_bufs;
};

struct scf_co_thread_s
{
	int                epfd;

	scf_rbtree_t       timers;

	scf_list_t         tasks;
	int                nb_tasks;

	scf_co_task_t*     current;

	uint32_t           exit_flag;
};

void __async_exit();
void __async_msleep(int64_t msec);
int  __async_read (int fd, void* buf, size_t count, int64_t msec);
int  __async_write(int fd, void* buf, size_t count);
int  __async_connect(int fd, const struct sockaddr *addr, socklen_t addrlen, int64_t msec);
int  __async_loop();


int scf_co_thread_open (scf_co_thread_t** pthread);
int scf_co_thread_close(scf_co_thread_t*  thread);

int scf_co_thread_add_task(scf_co_thread_t* thread, scf_co_task_t* task);

int scf_co_thread_run(scf_co_thread_t*  thread);

int scf_co_task_alloc(scf_co_task_t** ptask, uintptr_t func_ptr, uintptr_t rdi, uintptr_t rsi, uintptr_t rdx, uintptr_t rcx);
void scf_co_task_free(scf_co_task_t* task);

int scf_async(uintptr_t funcptr, uintptr_t rdi, uintptr_t rsi, uintptr_t rdx);

#endif

