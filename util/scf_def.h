#ifndef SCF_DEF_H
#define SCF_DEF_H

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<assert.h>
#include<errno.h>
#include<time.h>
#include<unistd.h>

#if 1
#include<sys/time.h>
static inline int64_t gettime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000000LL + tv.tv_usec;
}
#endif

// sign-extend low 'src_bits' of src to 64 bits
static inline uint64_t scf_sign_extend(uint64_t src, int src_bits)
{
	uint64_t sign = src >> (src_bits - 1) & 0x1;
	uint64_t mask = (~sign + 1) << (src_bits - 1);

	src |= mask;
	return src;
}

// zero-extend low 'src_bits' of src to 64 bits
static inline uint64_t scf_zero_extend(uint64_t src, int src_bits)
{
	uint64_t mask = (1ULL << src_bits) - 1;

	src &= mask;
	return src;
}

#define SCF_XCHG(x, y) \
	do {\
		typeof(x) tmp = x; \
		x = y; \
		y = tmp; \
	} while (0)

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

