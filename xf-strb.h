/**
 * DOC: xf-strb.h
 * Functions for constructing strings.
 *
 * Header version is accessible via %_XF_STRB_H where the three version numbers
 * are comma-separated.
 *
 * To externally share these functions between units(as non-static), define
 * %_XF_STATIC 0 before including the header and separately compile xf-strb.c.
 *
 * To specify the function declaration flags (static, extern, inline and
 * whatnot), define %_XF_FNC_DECLR. This defaults to static if %_XF_STATIC is 0,
 * and no declaration keywords if it is not 0.
 */
#ifndef _XF_STRB_H
#define _XF_STRB_H 00,02,11

/* #define _XF_STATIC 0 to use these as external functions */
#ifndef _XF_STATIC // Whether library should "#include" function bodies
#define XFSTATIC 1
#else
#define XFSTATIC _XF_STATIC
#endif

#if __GNUC__ /* Suppress unused warnings */
#define XFNOWRN __attribute__((unused))
#else
#define XFNOWRN
#endif

#ifdef _XF_FNC_DECLR // The flags embedded in function declaration
#define XFFNC XFNOWRN _XF_FNC_DECLR
#elif XFSTATIC == 1
#define XFFNC XFNOWRN static
#else
#define XFFNC XFNOWRN
#endif

#ifndef XF_STRB_EXPANDFNC
/**
 * XF_STRB_EXPANDFNC() - function used for expanding the array
 * @a:		the previous uint value to expand on
 *
 * Unless this macro is defined before the xf-strb.h is included or xf-strb.c
 * compiled, the definition is 2 * @a.
 *
 * If the value yielded by this macro is less than what was required,
 * then the required value will be used instead.
 *
 * Return:	The new, expanded value of type unsigned int.
 */
#define XF_STRB_EXPANDFNC(a) 2*a
#endif

/**
 * struct xf_strb - structure containing variable-size string info
 * @a:		the null terminated array of characters containing the string;
 * 		this must be allocated using alloc() or realloc()
 * @size:	total memory allocated for @a
 * @length:	the length of the array @a equal to strlen() @a + 1; this must
 * 		include the '\0' terminator
 *
 * Preferrably initialize the structure with xf_strb_construct() and free with
 * xf_strb_destruct().
 */
struct xf_strb
{
	char *a;
	unsigned int size;
	/* includes NULL terminator ( length of array ) */
	unsigned int length;
};

/**
 * xf_strb_construct() - initializes given instance of string buffer
 * @b:		the &struct xf_strb instance to initialize
 * @initsize:	initial amount of characters(plus null terminator) the buffer
 * 		should hold, must be equal to or larger than 1
 *
 * Note that calling this multiple times without calling xf_strb_destruct() for
 * the struct after each call will cause a memory leak.
 *
 * Return:	The reference to the struct just initialized(@b).
 */
XFFNC struct xf_strb *xf_strb_construct(struct xf_strb *b,
		unsigned int initsize);

/**
 * xf_strb_destruct() - releases memory associated with given strb
 * @b:		buffer instance to release
 *
 * Calling this function for a &struct xf_strb that has not been initialized
 * results in undefined behaviour.
 */
XFFNC void xf_strb_destruct(struct xf_strb *b);

/**
 * xf_strb_clear() - remove all text from buffer
 * @b:		buffer instance to modify
 */
XFFNC void xf_strb_clear(struct xf_strb *b);

/**
 * xf_strb_delete() - deletes characters from the buffer
 * @b:		the buffer instance to modify
 * @start:	the first character to erase
 * @length:	how many characters to erase
 */
XFFNC void xf_strb_delete(struct xf_strb *b, int start, int length);

/**
 * xf_strb_rmwhite() - trims the string start and end of whitespaces
 * @b:		&struct xf_strb instance to trim whitespaces from
 */
XFFNC void xf_strb_rmwhite(struct xf_strb *b);

/**
 * xf_strb_arrlen() - cut off the end of the string to give the string array a
 * 		new length
 * @b:		buffer instance to modify
 * @nlen:	new buffer length(including null terminator)
 *
 * This doesn't allocate more memory, just modifies the buffer length and sets
 * @b->a[@nlen - 1] to '\0'.
 */
XFFNC void xf_strb_arrlen(struct xf_strb *b, unsigned int nlen);

/**
 * xf_strb_clip() - cut off the end of the string from specified start
 * @b:		buffer instance to modify
 * @start:	the starting index of characters to be clipped
 *
 * Sets a new length of the string (strlen() sense).
 * @b->a[@start] will equal to '\0' and @b->length will be @start + 1.
 */
XFFNC void xf_strb_clip(struct xf_strb *b, int start);

/**
 * xf_strb_expand() - ensures that given buffer will hold given amount of characters
 * @b:		buffer instance
 * @size:	at least how large should the buffer be (incl. null terminator)
 */
XFFNC void xf_strb_expand(struct xf_strb *b, unsigned int size);

/**
 * xf_strb_shrink() - ensures given buffer won't hold more chars than given
 * @b:		buffer instance
 * @size:	maximum allowed size
 *
 * If the memory block @b->a is larger than @size, then it will be shortened to
 * hold @size char's.
 *
 * If @b->length is larger than @size, xf_strb_arrlen() will be called.
 */
XFFNC void xf_strb_shrink(struct xf_strb *b, unsigned int size);

/**
 * xf_strb_set() - replace the contents of given buffer
 * @b:		buffer which contents to modify
 * @s:		new content to set
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_set(struct xf_strb *b, const char *s);

/**
 * xf_strb_setf() - formatted replace of given buffer's contents
 * @b:		buffer which contents to modify
 * @format:	printf() style format string
 * @...:	variables pointed to by @format
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_setf(struct xf_strb *b, const char *format, ...)
	__attribute__((format(printf,2,3)));

#ifdef _STDARG_H
/**
 * xf_strb_vsetf() - replace the buffer contents va_list arguments
 * @b:		the target buffer whose contents to modify
 * @format:	printf() style format string describing @v
 * @v:		variable argument list
 *
 * Return:	Amount of characters written to buffer (not counting '\0'
 *		string terminator).
 */
XFFNC int xf_strb_vsetf(struct xf_strb *b, const char *format, va_list v);
#endif

/**
 * xf_strb_append() - add given string to the end of the buffer
 * @b:		buffer which contents to modify
 * @s:		string to add
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_append(struct xf_strb *b, const char *s);

/**
 * xf_strb_appendf() - add formatted string to the end of the buffer
 * @b:		buffer which contents to modify
 * @format:	printf() style format string
 * @...:	variables specified by @format
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_appendf(struct xf_strb *b, const char *format, ...)
	__attribute__((format(printf,2,3)));

#ifdef _STDARG_H
/**
 * xf_strb_vappendf() - add va_list formatted string to the end of buffer
 * @b:		buffer which contents to modify
 * @format:	printf() style format string
 * @v:		variable argument list
 *
 * Note: @v has to be initialized with va_start().
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_vappendf(struct xf_strb *b, const char *format, va_list v);
#endif

/**
 * xf_strb_prepend() - add given string to the beginning of the buffer
 * @b:		buffer which to modify
 * @s:		string to add
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_prepend(struct xf_strb *b, const char *s);

/**
 * xf_strb_prependf() - add formatted string to the beginning of the buffer
 * @b:		buffer to modify
 * @format:	printf() style format string to add to the buffer
 * @...:	variables specified by @format
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_prependf(struct xf_strb *b, const char *format, ...)
	__attribute__((format(printf,2,3)));

/**
 * xf_strb_insert() - inserts a string inside the buffer at given position
 * @b:		buffer instance to modify
 * @index:	where to insert the characters
 * @s:		the null terminated array of characters to insert
 *
 * Note: @index is byte-oriented, not actual characters' ('ä' advances 2 bytes
 * in UTF-8). Be warned that no encoding specific checks are made to ensure that
 * the buffer remains valid should @index be inside multibyte characters.
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_insert(struct xf_strb *b, int index, const char *s);
/**
 * xf_strb_insert() - inserts a formatted string inside the buffer at given
 * 		position
 * @b:		buffer instance to modify
 * @index:	where to insert the characters
 * @format:	the 'printf' format which to follow
 * @...:	variables specified by @format
 *
 * Note: @index is byte-oriented, not actual characters' ('ä' advances 2 bytes
 * in UTF-8). Be warned that no encoding specific checks are made to ensure that
 * the buffer remains valid should @index be inside multibyte characters.
 *
 * Return:	Amount of characters successfully written to buffer (doesn't
 * 		include '\0' terminator).
 */
XFFNC int xf_strb_insertf(struct xf_strb *b, int index,
		const char *format, ...)
__attribute__((format(printf,3,4)));

#if XFSTATIC == 1 // Include function bodies?
#include "xf-strb.c"
#endif

#if !defined(_XF_MACROS) || _XF_MACROS == 0
// Clear up some internal «local» definitions
#undef XFFNC
#undef XFNOWRN
#undef XFSTATIC
#undef XF_STRB_EXPANDFNC
#endif

#endif
