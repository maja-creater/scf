
void  scf__release_pt (void* objdata);

void* scf__auto_malloc(uintptr_t size);
void  scf__auto_ref(void* data);

void  scf__auto_freep (void** pp, scf__release_pt* release);
void  scf__auto_freep_array(void** pp, int nb_pointers, scf__release_pt* release);
void  scf__auto_free_array (void** pp, int size, int nb_pointers, scf__release_pt* release);

uintptr_t scf__strlen (const char *s);
int       scf__strcmp (const char *s1, const char *s2);
int       scf__strncmp(const char *s1, const char *s2,  uintptr_t n);
char*     scf__strncpy(char *dst,      const char *src, uintptr_t n);
int       scf__memcpy (void* dst,      const void* src, uintptr_t n);
int       scf__memcmp (void* dst,      const void* src, uintptr_t n);

int  scf_printf(const char* fmt, ...);


struct string
{
	uint8_t* data;
	int64_t  len;
	int64_t  capacity;

	int __init(string* this)
	{
		this->data = scf__auto_malloc(16);
		if (!this->data)
			return -1;

		this->data[0]  = '\0';
		this->capacity = 16;
		this->len      = 0;

		return 0;
	}

	int __init(string* this, const char* s)
	{
		int64_t len = scf__strlen(s);

		this->data = scf__auto_malloc(len + 1);
		if (!this->data)
			return -1;

		scf__memcpy(this->data, s, len);

		this->data[len] = '\0';
		this->capacity  = len;
		this->len       = len;

		return 0;
	}

	int __init(string* this, const char* s, int64_t len)
	{
		this->data = scf__auto_malloc(len + 1);
		if (!this->data)
			return -1;

		scf__memcpy(this->data, s, len);

		this->data[len] = '\0';
		this->capacity  = len;
		this->len       = len;

		return 0;
	}

	int operator+=(string* this, string* that)
	{
		int64_t len = this->len + that->len;

		if (len >= this->capacity) {
			uint8_t* p = scf__auto_malloc(len + 16);
			if (!p)
				return -1;

			scf__memcpy(p, this->data, this->len);

			this->data = p;
			this->capacity = len + 15;
		}

		scf__memcpy(this->data + this->len, that->data, that->len);

		this->data[len] = '\0';
		this->len       = len;

		return 0;
	}

	string*, int operator+(string* this, string* that)
	{
		string* s;

		s = create string();
		if (!s)
			return NULL, -1;

		int64_t len = this->len + that->len;

		if (s->len < len) {
			s->data = scf__auto_malloc(len + 1);
			if (!s->data)
				return NULL, -1;
		}

		scf__memcpy(s->data,  this->data, this->len);
		scf__memcpy(s->data + this->len,  that->data, that->len);

		s->data[len] = '\0';
		s->capacity  = len;
		s->len       = len;

		return s, 0;
	}

	int operator==(string* this, string* that)
	{
		if (this->len < that->len)
			return -1;
		else if (this->len > that->len)
			return 1;
		return scf__memcmp(this->data, that->data, this->len);
	}

	void __release(string* this)
	{
		if (this->data)
			scf__auto_freep(&this->data, NULL);
	}
};

int main()
{
	string* s0;
	string* s1;
	string* s2;
	string* s3;
	int err;

	s0 = "hello";
	s1 = " world";

	s2 = s0 + s1;
	s3 = "hello";

	scf_printf("### s0 == s3: %d\n", s0 == s3);
	return 0;
}

