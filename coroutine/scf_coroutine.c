#include"scf_coroutine.h"

static scf_co_thread_t* co_thread = NULL;

int  __scf_co_task_run(scf_co_task_t* task);
void __asm_co_task_yield(scf_co_task_t* task, uintptr_t* task_rip, uintptr_t* task_rsp, uintptr_t task_rsp0);

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

int scf_co_task_alloc(scf_co_task_t** ptask, uintptr_t func_ptr, uintptr_t rdi, uintptr_t rsi, uintptr_t rdx, uintptr_t rcx)
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

	task->rip = func_ptr;

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

int scf_async(uintptr_t funcptr, uintptr_t rdi, uintptr_t rsi, uintptr_t rdx)
{
	if (!co_thread) {
		if (scf_co_thread_open(&co_thread) < 0) {
			scf_loge("\n");
			return -1;
		}
	}

	scf_co_task_t* task = NULL;

	if (scf_co_task_alloc(&task, funcptr, rdi, rsi, rdx, 0) < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_co_thread_add_task(co_thread, task);
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
			task->err = -ENOMEM;
			return -ENOMEM;
		}

		task->stack_data     = p;
		task->stack_capacity = task->stack_len;
	}

	memcpy(task->stack_data, (void*)task->rsp, task->stack_len);
	return 0;
}

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

static size_t _co_read_from_bufs(scf_co_task_t* task, void* buf, size_t count)
{
	scf_co_buf_t** pp = &task->read_bufs;

	size_t pos = 0;

	while (*pp) {
		scf_co_buf_t* b = *pp;

		size_t len = b->len - b->pos;

		if (len > count - pos)
			len = count - pos;

		memcpy(buf + pos, b->data + b->pos, len);

		pos    += len;
		b->pos += len;

		if (b->pos == b->len) {

			*pp = b->next;

			free(b);
			b = NULL;
		}

		if (pos == count)
			break;
	}

	return pos;
}

static int _co_read_to_bufs(scf_co_task_t* task)
{
	scf_co_buf_t*  b = NULL;

	while (1) {
		if (!b) {
			b = malloc(sizeof(scf_co_buf_t) + 4096);
			if (!b) {
				scf_loge("\n");
				return -ENOMEM;
			}

			b->len = 0;
			b->pos = 0;

			scf_co_buf_t** pp = &task->read_bufs;

			while (*pp)
				pp = &(*pp)->next;
			*pp = b;
		}

		int ret = read(task->fd, b->data + b->len, 4096 - b->len);
		if (ret < 0) {
			if (EINTR == errno)
				continue;
			else if (EAGAIN == errno)
				break;
			else {
				scf_loge("\n");
				return -errno;
			}
		}

		b->len += ret;
		if (4096 == b->len)
			b = NULL;
	}

	return 0;
}

static int _co_add_event(int fd, uint32_t events)
{
	scf_co_thread_t*   thread = co_thread;
	scf_co_task_t*     task   = thread->current;

	struct epoll_event ev;
	ev.events   = events | EPOLLET | EPOLLRDHUP;
	ev.data.ptr = task;

	if (epoll_ctl(thread->epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {

		if (EEXIST != errno) {
			scf_loge("\n");
			return -errno;
		}

		if (epoll_ctl(thread->epfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
			scf_loge("\n");
			return -errno;
		}
	}

	task->fd     = fd;
	task->events = 0;
	return 0;
}

int __async_connect(int fd, const struct sockaddr *addr, socklen_t addrlen, int64_t msec)
{
	scf_co_thread_t*   thread = co_thread;
	scf_co_task_t*     task   = thread->current;

	int flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	while (1) {
		int ret = connect(fd, addr, addrlen);
		if (ret < 0) {
			if (EINTR == errno)
				continue;
			else if (EINPROGRESS == errno)
				break;
			else {
				scf_loge("\n");
				return -errno;
			}
		} else
			return 0;
	}

	if (task->time > 0)
		scf_rbtree_delete(&thread->timers, &task->timer);

	int64_t time = scf_gettime() + msec * 1000LL;
	task->time   = time;

	scf_rbtree_insert(&thread->timers, &task->timer, _co_timer_cmp);

	int ret = _co_add_event(fd, EPOLLOUT);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	__asm_co_task_yield(task, &task->rip, &task->rsp, task->rsp0);

	int err = -ETIMEDOUT;

	if (task->events & EPOLLOUT) {

		socklen_t errlen = sizeof(err);

		ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
		if (ret < 0) {
			scf_loge("ret: %d, errno: %d\n", ret, errno);
			err = ret;
		} else {
			err = -err;
		}

		scf_loge("err: %d\n", err);
	}

	if (epoll_ctl(thread->epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		scf_loge("EPOLL_CTL_DEL fd: %d, errno: %d\n", fd, errno);
		return -errno;
	}

	if (task->time > 0) {
		scf_loge("\n");
		scf_rbtree_delete(&thread->timers, &task->timer);
		task->time = 0;
	}

	return err;
}

int __async_read(int fd, void* buf, size_t count, int64_t msec)
{
	scf_co_thread_t*   thread = co_thread;
	scf_co_task_t*     task   = thread->current;

	int ret = _co_add_event(fd, EPOLLIN);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	if (task->time > 0)
		scf_rbtree_delete(&thread->timers, &task->timer);

	int64_t time = scf_gettime() + msec * 1000LL;
	task->time   = time;

	scf_rbtree_insert(&thread->timers, &task->timer, _co_timer_cmp);

	size_t pos = 0;

	while (1) {
		int ret = _co_read_to_bufs(task);
		if (ret < 0) {
			scf_loge("\n");
			return ret;
		}

		size_t len = _co_read_from_bufs(task, buf + pos, count - pos);
		pos += len;

		if (pos == count)
			break;

		if (time < scf_gettime())
			break;

		__asm_co_task_yield(task, &task->rip, &task->rsp, task->rsp0);
	}

	if (epoll_ctl(thread->epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		scf_loge("EPOLL_CTL_DEL fd: %d, errno: %d\n", fd, errno);
		return -errno;
	}

	return pos;
}

int __async_write(int fd, void* buf, size_t count)
{
	scf_co_thread_t*   thread = co_thread;
	scf_co_task_t*     task   = thread->current;

	int ret = _co_add_event(fd, EPOLLOUT);
	if (ret < 0) {
		scf_loge("\n");
		return ret;
	}

	size_t pos = 0;

	while (pos < count) {
		int ret = write(task->fd, buf + pos, count - pos);
		if (ret < 0) {
			if (EINTR == errno)
				continue;
			else if (EAGAIN == errno)
				__asm_co_task_yield(task, &task->rip, &task->rsp, task->rsp0);
			else {
				scf_loge("\n");
				return -errno;
			}
		} else
			pos += ret;
	}

	if (epoll_ctl(thread->epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		scf_loge("EPOLL_CTL_DEL fd: %d, errno: %d\n", fd, errno);
		return -errno;
	}

	return pos;
}

void __async_exit()
{
	if (co_thread)
		co_thread->exit_flag = 1;
}

int __async_loop()
{
	return scf_co_thread_run(co_thread);
}

#define SCF_CO_TASK_DELETE(task) \
	do { \
		if (task->time > 0) { \
			scf_rbtree_delete(&thread->timers, &task->timer); \
			task->time = 0; \
		} \
		\
		scf_list_del(&task->list); \
		thread->nb_tasks--; \
		\
		scf_co_task_free(task); \
		task = NULL; \
	} while (0)

int scf_co_thread_run(scf_co_thread_t* thread)
{
	if (!thread)
		return -EINVAL;

	while (!thread->exit_flag) {

		assert(thread->nb_tasks >= 0);

		if (0 == thread->nb_tasks)
			break;

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

			if (ret2 < 0 || SCF_CO_OK == ret2) {
				scf_loge("ret2: %d, thread: %p, task: %p\n", ret2, thread, task);

				SCF_CO_TASK_DELETE(task);
			}
		}

		while (1) {
			scf_co_task_t* task = (scf_co_task_t*) scf_rbtree_min(&thread->timers, thread->timers.root);
			if (!task) {
				scf_loge("\n");
				break;
			}

			if (task->time > scf_gettime()) {
				break;
			}

			scf_rbtree_delete(&thread->timers, &task->timer);
			task->time = 0;

			int ret2 = __scf_co_task_run(task);

			if (ret2 < 0 || SCF_CO_OK == ret2) {
				scf_loge("ret2: %d, thread: %p, task: %p\n", ret2, thread, task);

				SCF_CO_TASK_DELETE(task);
			}
		}

		free(events);
		events = NULL;
	}

	scf_loge("async exit\n");

	return 0;
}

