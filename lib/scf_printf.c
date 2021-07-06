struct va_list
{
	uint8_t*  iptr;
	uint8_t*  fptr;
	uint8_t*  optr;

	intptr_t  ireg;
	intptr_t  freg;
	intptr_t  others;
};

int scf__write(int fd, uint8_t* buf, uint64_t size);

int scf_ulong2a(char* buf, int* pn, int size, uint64_t num)
{
	int n = *pn;
	int i = n;

	while (n < size - 1) {

		buf[n++] = num % 10 + '0';

		num /= 10;
		if (0 == num)
			break;
	}

	*pn = n--;

	while (i < n) {
		char c   = buf[i];
		buf[i++] = buf[n];
		buf[n--] = c;
	}
	return *pn;
}

int scf_long2a(char* buf, int* pn, int size, int64_t num)
{
	int n = *pn;

	if (n < size - 1 && num < 0) {
		buf[n++] = '-';
		num = -num;
	}

	*pn = n;
	return scf_ulong2a(buf, pn, size, num);
}

int scf_hex2a(char* buf, int* pn, int size, uint64_t num)
{
	int n = *pn;
	int i = n;

	while (n < size - 1) {

		uint8_t x = num % 16;

		if (x > 9)
			buf[n++] = x - 10 + 'a';
		else
			buf[n++] = x + '0';

		num /= 16;
		if (0 == num)
			break;
	}

	*pn = n--;

	while (i < n) {
		char c   = buf[i];
		buf[i++] = buf[n];
		buf[n--] = c;
	}
	return *pn;
}

int scf_hex2a_prefix(char* buf, int* pn, int size, uint64_t num)
{
	int n = *pn;

	if (n < size - 1 - 2) {
		buf[n++] = '0';
		buf[n++] = 'x';
	}

	*pn = n;
	return scf_hex2a(buf, pn, size, num);
}

int scf_p2a(char* buf, int* pn, int size, uint64_t num)
{
	if (0 == num) {
		int   n = *pn;
		char* p = "null";

		while (n < size - 1 && *p)
			buf[n++] = *p++;

		*pn = n;
		return n;
	}

	return scf_hex2a_prefix(buf, pn, size, num);
}

int scf_str2a(char* buf, int* pn, int size, const char* str)
{
	int n = *pn;

	while (n < size - 1 && *str)
		buf[n++] = *str++;

	*pn = n;
	return n;
}

int scf_double2a(char* buf, int* pn, int size, double num)
{
	if (*pn < size - 1 && num < 0.0) {
		buf[(*pn)++] = '-';
		num = -num;
	}

	int64_t l    = (int64_t)num;
	int64_t diff = (int64_t)((num - l) * 1000000);

	scf_ulong2a(buf, pn, size, l);

	if (*pn < size - 1)
		buf[(*pn)++] = '.';

	return scf_ulong2a(buf, pn, size, diff);
}

int scf_vsnprintf(char* buf, int size, const char* fmt, va_list* ap)
{
	int n = 0;

	while (*fmt) {

		if ('%' != *fmt) {
			buf[n++] = *fmt++;
			continue;
		}

		fmt++;
		if ('%' == *fmt) {
			buf[n++] = *fmt++;
			continue;
		}

		int prefix = 0;
		if ('#' == *fmt) {
			prefix++;
			fmt++;
		}

		int l = 0;
		if ('l' == *fmt) {
			l++;
			fmt++;
		}

		if ('c' == *fmt)
			buf[n++] = va_arg(ap, int32_t);

		else if ('u' == *fmt) {
			if (l)
				scf_ulong2a(buf, &n, size, va_arg(ap, uint64_t));
			else
				scf_ulong2a(buf, &n, size, va_arg(ap, uint32_t));

		} else if ('d' == *fmt) {
			if (l)
				scf_long2a(buf, &n, size, va_arg(ap, int64_t));
			else
				scf_long2a(buf, &n, size, va_arg(ap, int32_t));

		} else if ('x' == *fmt) {
			if (prefix) {
				if (l)
					scf_hex2a_prefix(buf, &n, size, va_arg(ap, uint64_t));
				else
					scf_hex2a_prefix(buf, &n, size, va_arg(ap, uint32_t));
			} else {
				if (l)
					scf_hex2a(buf, &n, size, va_arg(ap, uint64_t));
				else
					scf_hex2a(buf, &n, size, va_arg(ap, uint32_t));
			}
		} else if ('p' == *fmt)
			scf_p2a(buf, &n, size, va_arg(ap, uint64_t));

		else if ('s' == *fmt)
			scf_str2a(buf, &n, size, va_arg(ap, char*));

		else if ('f' == *fmt) {
			if (l)
				scf_double2a(buf, &n, size, va_arg(ap, double));
			else
				scf_double2a(buf, &n, size, va_arg(ap, float));
		} else
			return -1;

		fmt++;
	}

	buf[n] = '\0';
	return n;
}

int scf_printf(const char* fmt, ...)
{
	va_list ap;

	char buf[1024];

	va_start(ap, fmt);
	int ret = scf_vsnprintf(buf, sizeof(buf) - 1, fmt, &ap);
	va_end(ap);

	if (ret > 0) {
		ret = scf__write(1, buf, ret);
	}

	return ret;
}

