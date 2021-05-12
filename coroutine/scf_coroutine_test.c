#include"scf_coroutine.h"

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include<sys/epoll.h>
#include<sys/time.h>
#include<fcntl.h>

static int _async_test(int n0, int n1, int n2, int n3)
{
	scf_logd("n0: %d, %d, %d, %d\n", n0, n1, n2, n3);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		scf_loge("\n");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(2000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int ret = __async_connect(fd, (struct sockaddr*)&addr, sizeof(addr), 1000);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = __async_write(fd, "hello\n", 7);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	scf_loge("async test ok, ret: %d\n", ret);
	return 0;
}

int main()
{
	scf_co_thread_t* thread = NULL;
	scf_co_task_t*   task   = NULL;

	int ret = scf_co_thread_open(&thread);
	if (ret < 0) {
		scf_loge("\n");
		return -1;
	}

	ret = scf_co_task_alloc(&task, (uintptr_t)_async_test, 5, 4, 3, 2);
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

