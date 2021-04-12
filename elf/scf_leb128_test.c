#include"scf_leb128.h"

int main()
{
	uint32_t a = 1253;
	int32_t  b = -7215;

	uint8_t buf[64];

	int ret = scf_leb128_encode_uint32(a, buf, 64);
	assert(ret > 0);

	size_t  len = 0;
	uint32_t c = scf_leb128_decode_uint32(buf, &len);
	scf_logi("ret: %d, c: %u, a: %u, len: %ld\n", ret, c, a, len);

	ret = scf_leb128_encode_int32(b, buf, 64);
	assert(ret > 0);

	int32_t d = scf_leb128_decode_int32(buf, &len);
	scf_logi("ret: %d, d: %d, b: %d, len: %ld\n", ret, d, b, len);

	return 0;
}

