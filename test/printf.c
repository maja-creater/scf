
int scf_printf(const char* fmt, ...);

int main()
{
	char buf[1024];

	float   f   = 2.71828;
	double  d   = -3.1415926;
	int64_t i64 = -255;

	int ret = scf_printf("i: %d, ld: %ld,  x: %x, x: %#lx, p: %p, s: %s, f: %f, d: %lf\n",
			100, i64, 254, 253, buf, "hello", f, -3.14);

	return ret;
}

