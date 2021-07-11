#ifndef SCF_LEB128_H
#define SCF_LEB128_H

#include"scf_def.h"

static inline int scf_leb128_encode_uint32(uint32_t value, uint8_t* buf, size_t len)
{
	size_t i = 0;

	uint8_t byte;

	do {
		byte    = value & 0x7f;
		value >>= 7;

		if (value)
			byte |= 0x80;

		if (i < len)
			buf[i++] = byte;
		else
			return -1;
	} while (value);

	return i;
}

static inline uint32_t scf_leb128_decode_uint32(const uint8_t* buf, size_t* plen)
{
	size_t i = 0;

	uint32_t byte;
	uint32_t value = 0;
	uint32_t shift = 0;

	while (1) {
		byte   = buf[i++];
		value |= (byte & 0x7f) << shift;

		if (!(byte & 0x80))
			break;

		shift += 7;
	};

	if (plen)
		*plen = i;

	return value;
}

static inline int scf_leb128_encode_int32(int32_t value, uint8_t* buf, size_t len)
{
	size_t i = 0;

	uint8_t byte;

	int more = 1;
	int neg  = value < 0;
	int size = sizeof(int32_t) << 3;

	while (more) {
		byte    = value & 0x7f;
		value >>= 7;

		if ((0 == value && !(byte & 0x40))
				|| (-1 == value && (byte & 0x40)))
			more = 0;
		else
			byte |= 0x80;

		if (i < len)
			buf[i++] = byte;
		else
			return -1;
	};

	return i;
}

static inline int32_t scf_leb128_decode_int32(const uint8_t* buf, size_t* plen)
{
	size_t i = 0;

	uint32_t byte;
	int32_t  value = 0;
	int32_t  shift = 0;

	int      size = sizeof(int32_t) << 3;

	while (1) {
		byte   = buf[i++];
		value |= (byte & 0x7f) << shift;
		shift += 7;

		if (!(byte & 0x80))
			break;
	};

	if (shift < size && (byte & 0x40))
		value |= -(1 << shift);

	if (plen)
		*plen = i;

	return value;
}

#endif

