
#if !defined(XFSTATIC) /* is .c processed first? */
#define _XF_STATIC 0 /* avoid looping between .c and .h */
#define _XF_MACROS 1
#include "xf-mregion.h"
#endif

#include <stdlib.h> // malloc free size_t
#include <assert.h> // assert

XFFNC struct xf_mregion *xf_mregion_create(size_t initsize)
{
	struct xf_mregion_sub *r = malloc(
			sizeof(struct xf_mregion_sub) + initsize);
	assert(r != NULL);
	r->next = NULL;
	r->size = initsize;
	r->length = 0;

	return (struct xf_mregion *) r;
}

XFFNC void xf_mregion_destroy(struct xf_mregion *r)
{
	struct xf_mregion_sub *nxt = &r->sub;
	struct xf_mregion_sub *s;

	while (nxt != NULL) {
		s = nxt;
		nxt = s->next;

		free(s);
	}
}

XFFNC void *xf_mregion_alloc(struct xf_mregion *r, size_t size)
{
	assert(r != NULL);
	assert(size > 0);
	struct xf_mregion_sub *s;
	for (s = &r->sub; s != NULL; s = s->next) {
		if (s->size - s->length < size)
			continue;
		void *rv = s->data + s->length;
		s->length += size;
		return rv;
	}
	/* Needs more memory */
	unsigned int total = 0;
	for (s = &r->sub; 0 == 0; s = s->next) {
		total += s->size;
		if (s->next == NULL) break;
	}
	unsigned int nsz = XF_MREGION_EXP_ALLOC(total, r->sub.size, s->size);
	if (nsz < size)
		nsz = size;

	s->next = malloc(sizeof(struct xf_mregion_sub) + nsz);
	assert(s->next != NULL);
	s = s->next;

	s->next = NULL;
	s->size = nsz;
	s->length = size;

	return s->data;
}

XFFNC void xf_mregion_clear(struct xf_mregion *r)
{
	struct xf_mregion_sub *s;

	for (s = &r->sub; s != NULL; s = s->next)
		s->length = 0;
}

XFFNC void xf_mregion_undo(struct xf_mregion *r, void *mem)
{
	struct xf_mregion_sub *s = &r->sub;

	for (s = &r->sub; s != NULL; s = s->next) {
		if (((char *)mem) < s->data || ((char *)mem) > s->data + s->length)
			continue;
		s->length = (unsigned int) ((char *)mem - s->data);
		return;
	}
	/* if control reaches this point, mem is not part of (active) region */
	assert(1==2);
	return;
}

XFFNC size_t xf_mregion_memcnt(struct xf_mregion *r)
{
	assert(r != NULL);
	struct xf_mregion_sub *s;
	size_t cnt = 0;
	for (s = &r->sub; s != NULL; s = s->next)
		cnt += s->size;

	return cnt;
}

