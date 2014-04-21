/**
 * DOC: xf-mregion.h
 * An implementation of region-based memory management.
 *
 * http://en.wikipedia.org/wiki/Region-based_memory_management
 *
 * DOC: README
 * Allocated memory will not loose its position: underneath a region consists
 * of a linked-list array of blocks (subregions), when initial memory is
 * exhausted, another subregion is allocated (as opposed to one contiguous block
 * being realloc()'ed to hold more).
 *
 * Header version is accessible via %_XF_MREGION_H where the three version
 * numbers are comma-separated.
 *
 * To externally share these functions between units (as non-static), define
 * %_XF_STATIC 0 before including the header and separately compile
 * xf-mregion.c.
 *
 * To specify the function declaration flags (static, extern, inline and
 * whatnot), define %_XF_FNC_DECLR. This defaults to static if %_XF_STATIC is 0,
 * and no declaration keywords if it is not 0.
 */

#ifndef _XF_MREGION_H
#define _XF_MREGION_H 00,02,11

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

#ifndef XF_MREGION_EXP_ALLOC
/**
 * XF_MREGION_EXP_ALLOC() - function used to get the size of a new subregion
 * @total:	total memory currently allocated for current region
 * @initsize:	first block's size
 * @previous:	size of the current last memory block
 *
 * Unless this macro is defined before the xf-mregion.h is included or
 * xf-mregion.c compiled, the definition is @initsize.
 *
 * If the value yielded by this macro is less than what was required,
 * then the required value will be used instead.
 *
 * Return:	The returned unsigned integer value will be used to
 * 		determine how much data the next subregion will be able to
 * 		hold.
 */
#define XF_MREGION_EXP_ALLOC(total,initsize,previous) initsize
#endif

/**
 * struct xf_mregion_sub - information associated with a subregion
 * @next:	the link to the next subregion or %NULL if this is the last
 * @size:	size of the @data array
 * @length:	length to which the @data array is used
 * @data:	memory for xf_mregion_alloc()
 */
struct xf_mregion_sub {
	struct xf_mregion_sub *next;
	unsigned int size;
	unsigned int length;
	char data[];
};

/**
 * struct xf_mregion - first link in memory region
 * @sub:	&struct xf_mregion_sub accessor
 *
 * This is separate from &struct xf_mregion_sub to make a distinction on the
 * beginning of the linked-list.
 */
struct xf_mregion {
	struct xf_mregion_sub sub;
};

/**
 * xf_mregion_create() - initializes an instance of region-based memory manager
 * @initsize:	bytes of data the initial memory subregion should be able
 * 		to hold
 *
 * Note that the &struct xf_mregion structures are always dynamically
 * allocated.
 */
XFFNC struct xf_mregion *xf_mregion_create(size_t initsize);

/**
 * xf_mregion_destroy() - releases all memory associated with a memory region
 * @r:		memory region to release
 */
XFFNC void xf_mregion_destroy(struct xf_mregion *r);

/**
 * xf_mregion_alloc() - allocate some memory from given region
 * @r:		region to allocate from
 * @size:	size of the allocation
 *
 * Further allocations from given region will not cause allocated memory to
 * shift location. (Guaranteed pointer relevance until xf_mregion_rewind() or
 * xf_mregion_destroy() is used to free given memory.)
 *
 */
XFFNC void *xf_mregion_alloc(struct xf_mregion *r, size_t size);

/**
 * xf_mregion_rewind() - rewind the latest allocations to region
 * @r:		region to rewind
 * @mem:	the memory to rewind the allocations to, as returned by
 * 		xf_mregion_alloc(); any memory that was allocated from given
 * 		region after that allocation, should not be used
 * 		(consider them free()'d aswell)
 *
 * Note that if given @mem is not the latest memory allocated from the region,
 * some of the memory for the later allocations may have been taken from
 * previous subregions, and so would become dead data until region is
 * destroyed or rewound more. This is because all previous subregions will
 * also be considered for memory to be allocated out of for maximal usage.
 */
XFFNC void xf_mregion_rewind(struct xf_mregion *r, void *mem);

/**
 * xf_mregion_pos() - return a placeholder for rewind
 * @r:		memory region whose memory to inspect
 *
 * Return:	returns the pointer to the outermost xf_mregion_alloc()'ed
 * 		space plus one
 */
XFFNC void *xf_mregion_pos(struct xf_mregion *r);

/**
 * xf_mregion_clear() - rewind all of the memory to be once again usable
 * @r:		region to clear
 *
 * After this function returns, all the memory regions will be still linked,
 * but marked as available for allocation via xf_mregion_alloc().
 */
XFFNC void xf_mregion_clear(struct xf_mregion *r);

/**
 * xf_htable_memcnt() - count dynamically allocated memory associated with region
 * @r:		memory region which's memory to examine
 *
 * Return:	amount of memory malloc()'ed
 */
XFFNC size_t xf_mregion_memcnt(struct xf_mregion *r);

#if XFSTATIC == 1 // Include function bodies?
#include "xf-mregion.c"
#endif

#if !defined(_XF_MACROS) || _XF_MACROS == 0
// Clear up some internal «local» definitions
#undef XFFNC
#undef XFNOWRN
#undef XFSTATIC
#undef XF_MREGION_EXP_ALLOC
#endif

#endif
