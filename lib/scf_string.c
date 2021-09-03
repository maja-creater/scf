
void  scf__release_pt (void* objdata);

void* scf__auto_malloc(uintptr_t size);
void  scf__auto_ref(void* data);

void  scf__auto_freep (void** pp, scf__release_pt* release);
void  scf__auto_freep_array(void** pp, int nb_pointers, scf__release_pt* release);
void  scf__auto_free_array (void** pp, int size, int nb_pointers, scf__release_pt* release);

uintptr_t strlen (const char *s);
int       strcmp (const char *s1, const char *s2);
int       strncmp(const char *s1, const char *s2,  uintptr_t n);
char*     strncpy(char *dst,      const char *src, uintptr_t n);
int       memcpy (void* dst,      const void* src, uintptr_t n);
int       memcmp (void* dst,      const void* src, uintptr_t n);

int       printf(const char* fmt, ...);


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
		int64_t len = strlen(s);

		this->data = scf__auto_malloc(len + 1);
		if (!this->data)
			return -1;

		memcpy(this->data, s, len);

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

		memcpy(this->data, s, len);

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

			memcpy(p, this->data, this->len);

			this->data = p;
			this->capacity = len + 15;
		}

		memcpy(this->data + this->len, that->data, that->len);

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

		memcpy(s->data,  this->data, this->len);
		memcpy(s->data + this->len,  that->data, that->len);

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
		return memcmp(this->data, that->data, this->len);
	}

	void __release(string* this)
	{
		if (this->data)
			scf__auto_freep(&this->data, NULL);
	}
};

