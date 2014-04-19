
#if !defined(XFSTATIC) /* is .c processed first? */
#define _XF_STATIC 0 /* Avoid looping between .c and .h */
#define _XF_MACROS 1
#include "xf-strb.h"
#endif

#include <string.h> /* memmove */
#include <stdlib.h> /* free, malloc */
#include <stdarg.h> /* va_list */
#include <stdio.h>
#include <assert.h> /* assert */


XFFNC struct xf_strb *xf_strb_construct(struct xf_strb *b,
		unsigned int initsize)
{
	assert(b != NULL);
	assert(initsize >= 1);
	b->size = initsize;
	b->length = 1;
	b->a = malloc(initsize);
	b->a[0] = '\0';
	return b;
}

XFFNC void xf_strb_destruct(struct xf_strb *b)
{
	assert(b != NULL);
	/*
	 * while not an actual error(from the point that it could be allowed as
	 * free(NULL) is allowed), multiple destructions make no sense and 
	 * point to code error 
	 */
	assert(b->length != 0); 
	free(b->a);
	b->a = NULL;
	b->size = 0;
	b->length = 0; /* determines that the struct is in fact uninitialized. */
}

XFFNC void xf_strb_expand(struct xf_strb *b, unsigned int l)
{
	assert(b != NULL);
	if (b->size >= l) return;
	b->size = XF_STRB_EXPANDFNC((b->size));
	if (b->size < l) b->size = l;
	b->a = realloc(b->a, b->size);
}

XFFNC void xf_strb_shrink(struct xf_strb *b, unsigned int l)
{
	assert(b != NULL);
	if (b->size <= l) return;
	if (l < b->length)
		xf_strb_arrlen(b, l);
	b->size = l;
	b->a = realloc(b->a, l);
}

XFFNC void xf_strb_arrlen(struct xf_strb *b, unsigned int nlen)
{
	assert(b != NULL);
	assert(b->length >= nlen);
	b->length = nlen;
	b->a[nlen - 1] = '\0';
}

XFFNC void xf_strb_clip(struct xf_strb *b, int start)
{
	assert(b != NULL);
	assert(b->length >= start + 1);
	b->length = start + 1;
	b->a[start] = '\0';
}

XFFNC int xf_strb_set(struct xf_strb *b, const char *s)
{
	assert(b != NULL);
	assert(s != NULL);
	int slen = strlen(s) + 1;
	xf_strb_expand(b, slen);
	memcpy(b->a, s, slen);
	b->length = slen;
	return slen - 1;
}

XFFNC int xf_strb_setf(struct xf_strb *b, const char *format, ...)
{
	assert(b != NULL);
	assert(format != NULL);
	va_list l;
	va_start(l, format);
	int r;
	r=vsnprintf(b->a, b->size, format, l) + 1;/* +1 for NULL terminator */
	if (r > b->size) {
		xf_strb_expand(b, r);
		va_end(l);
		va_start(l, format);
		vsnprintf(b->a, b->size, format, l);
	}
	b->length = r; 

	va_end(l);
	return r - 1;
}

XFFNC int xf_strb_append(struct xf_strb *b, const char *s)
{
	assert(b != NULL);
	assert(s != NULL);
	unsigned int slen = strlen(s);
	xf_strb_expand(b, b->length + slen);
	memcpy(b->a + (b->length - 1), s, slen + 1);
	b->length = b->length + slen;
	return slen;
}

XFFNC int xf_strb_appendf(struct xf_strb *b, const char *format, ...)
{
	assert(b != NULL);
	assert(format != NULL);
	va_list l;
	va_start(l, format);
	int r; 
	/* vsnprintf's return val doesn't incl. \0, however n(2nd arg) does */
	r = vsnprintf(b->a + b->length - 1, b->size - b->length + 1, format, l);

	/* cast to int so sz0-len1 doesnt overflow */
	if (r > (int) (b->size - b->length)) { 
		/* size and length both contain NULL terminator length, */
		/* so their difference doesn't */
		xf_strb_expand(b, b->length + r + 0);
		va_end(l);
		va_start(l, format);
		r = vsnprintf(b->a + b->length - 1, b->size - b->length + 1, format, l);
	}
	b->length += r;

	va_end(l);
	return r;
}

XFFNC int xf_strb_vappendf(struct xf_strb *b, const char *format, va_list l)
{
	assert(b != NULL);
	assert(format != NULL);

	va_list cpy;
	va_copy(cpy, l);
	int r; 
	/* vsnprintf's return val doesn't incl. \0, however n(2nd arg) does */
	r = vsnprintf(b->a + b->length - 1, b->size - b->length + 1, format, l);

	/* cast to int so sz0-len1 doesnt overflow */
	if (r > (int) (b->size - b->length)) { 
		/* size and length both contain NULL terminator length, */
		/* so their difference doesn't */
		xf_strb_expand(b, b->length + r);
		r = vsnprintf(b->a + b->length - 1, b->size - b->length + 1, format, cpy);
	}
	b->length += r;

	va_end(l);
	return r;
}

XFFNC int xf_strb_prepend(struct xf_strb *b, const char *s)
{
	assert(b != NULL);
	assert(s != NULL);
	unsigned int slen = strlen(s);
	xf_strb_expand(b, b->length + slen);
	memmove(b->a + slen, b->a, b->length);
	memcpy(b->a, s, slen);
	b->length += slen;
	return slen;
}

XFFNC int xf_strb_prependf(struct xf_strb *b, const char *format, ...)
{
	assert(b != NULL);
	assert(format != NULL);
	char tmp[24];
	va_list l;
	va_start(l, format);
	int r = vsnprintf(tmp, 24, format, l);

	xf_strb_expand(b, b->length + r);
	memmove(b->a + r, b->a, b->length);

	if (r + 1 > 24) {
		tmp[0] = b->a[r];
		va_end(l);
		va_start(l, format);
		vsnprintf(b->a, r + 1, format, l);
		b->a[r] = tmp[0];
	} else {
		memcpy(b->a, tmp, r);
	}
	return r;
}

XFFNC int xf_strb_insertf(struct xf_strb *b, int index,
		const char *format, ...)
{
	assert(b != NULL);
	assert(format != NULL);
	assert(index < b->length);
	char tmp[24];
	int r;
	va_list l;

	va_start(l, format);
	r = vsnprintf(tmp, 24, format, l);

	xf_strb_expand(b, b->length + r);
	memmove(b->a + index + r, b->a + index, b->length - index);
	if (r + 1 > 24) {
		va_end(l);
		va_start(l, format);
		vsnprintf(b->a + index, b->size - index, format, l);
	} else {
		memcpy(b->a + index, tmp, r);
	}

	va_end(l);

	b->length += r;
	return r;
} 

XFFNC int xf_strb_insert(struct xf_strb *b, int index,
		const char *s)
{
	assert(b != NULL);
	assert(s != NULL);
	assert(index < b->length);
	int len_rest = b->length - index;
	int len_s = strlen(s);
	xf_strb_expand(b, index + len_rest + len_s);
	memmove(b->a + index + len_s, b->a + index, len_rest);
	memcpy(b->a + index, s, len_s);
	b->length += len_s;
	return len_s;
}

XFFNC void xf_strb_clear(struct xf_strb *b)
{
	assert(b != NULL);
	b->length = 1;
	b->a[0] = '\0';
}

XFFNC void xf_strb_rmwhite(struct xf_strb *b)
{
	(void) xf_strb_rmwhite;
	assert(b != NULL);
	int i;
	for (i = 0; i < b->length; i++)
	{
		if (*(b->a + i) == 32 || *(b->a + i) == '\t' || *(b->a + i) == '\n' 
				|| *(b->a + i) == '\r' ) continue;
		if (i == 0) break;
		b->length -= i;
		memmove(b->a, b->a + i, b->length);
		break;
	}
	if (i == b->length) 
	{
		b->length = 1;
		b->a[0] = '\0';
		return;
	}
	/* "o \0" */
	for (i = b->length - 2; i >= 0; i--)
	{
		if(*(b->a+i) == 32 || *(b->a+i) == '\t' || *(b->a+i) == '\n' ||
				*(b->a+i) == '\r' ) continue;
		b->length=i+2;
		b->a[i+1] = '\0';
		break;
	}
}

