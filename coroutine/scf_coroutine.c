#include"scf_coroutine.h"

static scf_co_thread_t* co_thread = NULL;

static void _async_test(int n0, int n1, int n2, int n3);

int64_t scf_gettime()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	int64_t time = tv.tv_sec * 1000LL * 1000LL + tv.tv_usec;

	return time;
}

int scf_co_thread_open (scf_co_thread_t** pthread)
{
	scf_co_thread_t* thread = calloc(1, sizeof(scf_co_thread_t));
	if (!thread)
		return -ENOMEM;

	thread->epfd = epoll_create(1024);
	if (-1 == thread->epfd) {
		scf_loge("errno: %d\n", errno);
		return -1;
	}

	scf_rbtree_init(&thread->timers);
	scf_list_init  (&thread->tasks);

	co_thread = thread;

	*pthread = thread;
	return 0;
}

int scf_co_thread_close(scf_co_thread_t*  thread)
{
	scf_loge("\n");
	return -1;
}

int scf_co_task_alloc(scf_co_task_t** ptask, uintptr_t rdi, uintptr_t rsi, uintptr_t rdx, uintptr_t rcx)
{
	scf_co_task_t* task = calloc(1, sizeof(scf_co_task_t));
	if (!task)
		return -ENOMEM;

	task->stack_data = calloc(4, sizeof(uintptr_t));
	if (!task->stack_data) {
		free(task);
		return -ENOMEM;
	}

	task->stack_len = 4 * sizeof(uintptr_t);

	uintptr_t* stack = task->stack_data;
	stack[0] = rdi;
	stack[1] = rsi;
	stack[2] = rdx;
	stack[3] = rcx;

	scf_loge("rdi: %ld, stack[0]: %ld\n", rdi, stack[0]);

	task->stack_capacity = task->stack_len;

	task->rip = (uintptr_t)_async_test;

	*ptask = task;
	return 0;
}

static int _co_timer_cmp(scf_rbtree_node_t* node0, void* data)
{
	scf_co_task_t* task0 = (scf_co_task_t*)node0;
	scf_co_task_t* task1 = (scf_co_task_t*)data;

	if (task0->time < task1->time)
		return -1;
	else if (task0->time > task1->time)
		return 1;
	return 0;
}

static int _co_timer_cmp_task(scf_rbtree_node_t* node0, void* data)
{
	scf_co_task_t* task0 = (scf_co_task_t*)node0;
	scf_co_task_t* task1 = (scf_co_task_t*)data;

	if (task0->time < task1->time)
		return -1;
	else if (task0->time > task1->time)
		return 1;
}

void scf_co_task_free(scf_co_task_t* task)
{
	if (task->stack_data)
		free(task->stack_data);

	free(task);
}

int scf_co_thread_add_task(scf_co_thread_t* thread, scf_co_task_t* task)
{
	task->time = scf_gettime();

	scf_rbtree_insert(&thread->timers, &task->timer, _co_timer_cmp);

	scf_list_add_tail(&thread->tasks,  &task->list);

	task->thread = thread;

	thread->nb_tasks++;
	return 0;
}

int __save_stack(scf_co_task_t* task)
{
	task->stack_len = task->rsp0 - task->rsp;

	scf_loge("task: %p, stack_len: %ld, start: %#lx, end: %#lx, task->rip: %#lx\n",
			task, task->stack_len, task->rsp, task->rsp0, task->rip);

	if (task->stack_len > task->stack_capacity) {
		void* p = realloc(task->stack_data, task->stack_len);
		if (!p) {
			task->ret = -ENOMEM;
			return -ENOMEM;
		}

		task->stack_data     = p;
		task->stack_capacity = task->stack_len;
	}

	memcpy(task->stack_data, (void*)task->rsp, task->stack_len);
	return 0;
}

void __asm_co_task_yield(scf_co_task_t* task, uintptr_t* task_rip, uintptr_t* task_rsp, uintptr_t task_rsp0);

void __async_msleep(int64_t msec)
{
	scf_co_thread_t* thread = co_thread;
	scf_co_task_t*   task   = thread->current;

	if (task->time > 0)
		scf_rbtree_delete(&thread->timers, &task->timer);

	int64_t time = scf_gettime();
	task->time = time + 1000 * msec;

	scf_rbtree_insert(&thread->timers, &task->timer, _co_timer_cmp);

	scf_loge("task->rip: %#lx, task->rsp: %#lx\n", task->rip, task->rsp);

	__asm_co_task_yield(task, &task->rip, &task->rsp, task->rsp0);
}

static void _async_test(int n0, int n1, int n2, int n3)
{
	scf_logd("n0: %d, %d, %d, %d\n", n0, n1, n2, n3);

	scf_co_task_t* task = co_thread->current;

	scf_loge("task->rip: %#lx, task->rsp: %#lx\n", task->rip, task->rsp);
#if 1
	int i = 0;
	while (i < n0) {
		scf_loge("i: %d, n: %d\n", i, n0);

		__async_msleep(500);

		printf("\n");
		i++;
	}
#endif
}

uintptr_t __asm_co_task_run(uintptr_t* task_rsp, void* stack_data,
		uintptr_t task_rip, intptr_t stack_len, uintptr_t* task_rsp0);

int __scf_co_task_run(scf_co_task_t* task)
{
	task->thread->current = task;

	task->ret = 0;

	uintptr_t rsp1 = 0;
	scf_loge("task %p, rsp0: %#lx, rsp1: %#lx\n", task, task->rsp0, rsp1);

	rsp1 = __asm_co_task_run(&task->rsp, task->stack_data, task->rip,
			task->stack_len, &task->rsp0);

#if 1
	if (task->rsp0 == rsp1) {
		scf_loge("task %p run ok, rsp0: %#lx, rsp1: %#lx\n", task, task->rsp0, rsp1);

		if (task->time > 0) {
			scf_rbtree_delete(&task->thread->timers, &task->timer);
			task->time = 0;
		}

		scf_list_del(&task->list);
		task->thread->nb_tasks--;

		scf_co_task_free(task);
		task = NULL;

	} else {
		scf_loge("task %p running, rsp0: %#lx, rsp1: %#lx\n", task, task->rsp0, rsp1);
	}
#endif
	return 0;
}

int scf_co_thread_run(scf_co_thread_t* thread)
{
	if (!thread)
		return -EINVAL;

	scf_loge("thread: %p\n", thread);

	while (1) {

#if 1
		struct epoll_event* events = calloc(thread->nb_tasks + 1, sizeof(struct epoll_event));
		if (!events)
			return -ENOMEM;

		int ret = epoll_wait(thread->epfd, events, thread->nb_tasks + 1, 300);
		if (ret < 0) {
			scf_loge("errno: %d\n", errno);

			free(events);
			return -1;
		}
		int i;
		for (i = 0; i < ret; i++) {

			scf_co_task_t* task = events[i].data.ptr;
			assert(task);

			task->events = events[i].events;

			int ret2 = __scf_co_task_run(task);
			if (ret2 < 0) {
				scf_loge("ret2: %d, thread: %p, task: %p\n", ret2, thread, task);

				if (task->time > 0) {
					scf_rbtree_delete(&thread->timers, &task->timer);
					task->time = 0;
				}

				scf_list_del(&task->list);
				thread->nb_tasks--;

				scf_co_task_free(task);
				task = NULL;
			}
		}
#endif

		while (1) {
			scf_co_task_t* task = (scf_co_task_t*) scf_rbtree_min(&thread->timers, thread->timers.root);
			if (!task) {
				scf_loge("\n");
				break;
			}

			if (task->time > scf_gettime()) {
				scf_loge("\n");
				break;
			}

			scf_rbtree_delete(&thread->timers, &task->timer);
			task->time = 0;

			__scf_co_task_run(task);
		}
#if 1
		scf_loge("\n");

		free(events);
		events = NULL;
#endif
	}

	return 0;
}

#if 1
int main()
{
	scf_co_thread_t* thread = NULL;
	scf_co_task_t*   task   = NULL;

	int ret = scf_co_thread_open(&thread);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = scf_co_task_alloc(&task, 5, 4, 3, 2);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = scf_co_thread_add_task(thread, task);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_loge("thread: %p\n", thread);

	ret = scf_co_thread_run(thread);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	return 0;
}
#endif

