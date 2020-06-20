#ifndef SCF_DEF_H
#define SCF_DEF_H

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<assert.h>

#ifdef SCF_DEBUG
#define scf_logd(fmt, ...) printf("%s(), %d, "fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define scf_logd(fmt, ...)
#endif

#define scf_logi(fmt, ...) printf("%s(), %d, info: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define scf_loge(fmt, ...) printf("%s(), %d, \033[31m error:\033[0m "fmt, __func__, __LINE__, ##__VA_ARGS__)
#define scf_logw(fmt, ...) printf("%s(), %d, \033[33m warning:\033[0m "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define SCF_CHECK_ERROR(cond, ret, fmt, ...) \
	do { \
		if (cond) { \
			printf("%s(), %d, \033[31m error:\033[0m "fmt, __func__, __LINE__, ##__VA_ARGS__); \
			return ret; \
		} \
	} while (0)

#endif

