struct list
{
	list* prev;
	list* next;
};

inline void list_init(list* h)
{
	h->prev = h;
	h->next = h;
}

inline list* list_head(list* h)
{
	return h->next;
}

inline list* list_tail(list* h)
{
	return h->prev;
}

inline list* list_sentinel(list* h)
{
	return h;
}

inline list* list_next(list* n)
{
	return n->next;
}

inline list* list_prev(list* n)
{
	return n->prev;
}

inline void list_del(list* n)
{
	n->prev->next = n->next;
	n->next->prev = n->prev;
	n->prev = NULL;
	n->next = NULL;
}

inline int list_empty(list* h)
{
	return h->next == h;
}

inline void list_add_tail(list* h, list* n)
{
	h->prev->next = n;
	n->prev = h->prev;
	n->next = h;
	h->prev = n;
}

inline void list_add_front(list* h, list* n)
{
	h->next->prev = n;
	n->next = h->next;
	n->prev = h;
	h->next = n;
}

