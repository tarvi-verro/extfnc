/**
 * DOC:	xf-htable.h - an implementation of hash-table
 *
 * By convention, functions which report integer errors, return 0 on
 * success.
 *
 * Important note: keys are not copied!!
 */
#ifndef _XF_HTABLE_H
#define _XF_HTABLE_H 00,02,03

#include <stddef.h> // offsetof
#include <stdint.h> // uintN_t

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

#ifdef _XF_FNC_DECLR /* The flags embedded in function declaration */
#define XFFNC XFNOWRN _XF_FNC_DECLR
#elif XFSTATIC == 1 
#define XFFNC XFNOWRN static
#else
#define XFFNC XFNOWRN
#endif

#ifndef XF_HTABLE_EXPANDFNC
#define XF_HTABLE_EXPANDFNC(oldsize) \
	oldsize * 2
#endif

/**
 * enum - special function return values
 * @XF_HTABLE_ESUCCESS:	function returned successfully
 * @XF_HTABLE_EFULL:	bucket cannot hold more
 * @XF_HTABLE_ENOTFOUND:key not found in table
 * @XF_HTABLE_ENOTEQUAL:value is not equal what's in the table
 * @XF_HTABLE_ESET:	a value is already set for given key
 *
 * Note that the description of the given function will list all valid error
 * codes that it might return.
 */
enum {
	XF_HTABLE_ESUCCESS =	0,
	XF_HTABLE_EFULL =	1,
	XF_HTABLE_ENOTFOUND =	2,
	XF_HTABLE_ENOTEQUAL =	3,
	XF_HTABLE_ESET =	4,
};
/**
 * enum - key types macros 
 * @XF_HTABLE_KEY_INDIRECT:	key is accessible via pointer
 * @XF_HTABLE_KEY_DIRECT:	key is accessible directly
 */
enum {
	XF_HTABLE_KEY_INDIRECT = 0,
	XF_HTABLE_KEY_DIRECT =	 1,
};
/**
 * union xf_htable_key - holds a key or a pointer to it and length
 * @accesstyp:	either %XF_HTABLE_KEY_DIRECT or %XF_HTABLE_KEY_INDIRECT
 * @indirect:	use when @accesstyp is %XF_HTABLE_KEY_INDIRECT
 * @direct:	use when @accesstyp is %XF_HTABLE_KEY_DIRECT
 *
 * To avoid having fragmented memory(and cache misses? -- I've no idea if these 
 * points are actually valid anyway), keys are stored in place of pointers to
 * them if possible. On a system with 64-bit(8 byte) pointers, thanks to 
 * padding, there should be enough room to store character array 
 * "123456789012345" with its length in place of a pointer and length of the 
 * possibly larger key.
 *
 * However, with 4-byte pointers the room could be limited to hold "1234567"
 * and add no padding, you get "123456". At which point the code fork for 
 * shorter keys is perhaps less than ideal.
 *
 * Use the %XF_HTABLE_KEY_DIRECT_MAX macro to determine how many bytes can
 * @indirect.a hold.
 */
union xf_htable_key {
	uint8_t accesstyp : 1;
	struct {/* accesstyp 0 */
		uint8_t accesstyp : 1;
		uint16_t length;
		const void *ptr;
	} indirect; 
	struct { /* accesstyp 1 */
		uint8_t accesstyp : 1;
		uint8_t length : 7;
		uint8_t a[]; // 15 bytes on x86_64, 7 bytes w/ 32bit pointers
	} direct; 
};
/* how many bytes can xf_htable_key.direct.a hold */
#define XF_HTABLE_KEY_DIRECT_MAX \
	(offsetof(union xf_htable_key, indirect.ptr) + sizeof(void *) \
	 - offsetof(union xf_htable_key, direct.a))
/**
 * struct xf_htable_bucket - a array of key/value pairs
 * @length:	total members in @data
 * @size:	maximum members @data can hold
 * @data:	bytes holding an array for keys (offset 0) and an array for 
 * 		values (offset @size * sizeof() &union xf_htable_key )
 *
 * Values are stored in an array after @data, use the function 
 * xf_htable_bucket_val() to retrieve them.
 */
struct xf_htable_bucket {
	unsigned short length;
	unsigned short size;
	union xf_htable_key data[];
};

/**
 * struct xf_htable - instance of hash-table
 * @hash:	hash function used for distributing the data
 * @res_mask:	mask to bitwise AND out high bits to get a bucket's index;
 * 		@res_mask + 1 to get amount of buckets
 * @value_size:	how many bytes does a single value take up
 * @buckets:	list of buckets, a bucket slot may be %NULL if no entry has yet
 * 		been associated with it
 * 
 * Limitations:
 * 	Each bucket can maximally hold %USHRT_MAX entries. 
 *
 * 	Values must all take up the same size in the table, however you're free
 * 	to store a pointer as a value.
 */
struct xf_htable 
{
	uint32_t (*hash)(const char *key, int len);
	uint32_t res_mask;
	size_t value_size;
	struct xf_htable_bucket **buckets;	
};

/**
 * xf_htable_bucket_val() - access a value from given bucket's data
 * @tbl:	the hashtable the bucket's in
 * @b:		bucket to get the key from
 * @index:	position of value to get
 *
 * Return:	pointer to value at given bucket's given index
 */
static inline void *xf_htable_bucket_val(struct xf_htable *tbl, 
		struct xf_htable_bucket *b, int index)
{
	return ((uint8_t *)&b->data[0]) + (sizeof(union xf_htable_key) * b->size 
		+ tbl->value_size * index);

}
 
/**
 * xf_htable_construct() - initialize an instance of struct xf_htable
 * @t:		instance to initialize
 * @size_bits:	how many bits to use for bucket IDs;
 * 		2^(@size_bits + 1) = how many buckets to distribute the load over 
 * @value_size:	the byte-size of data you wish to associate with the keys
 * @hash:	the hash function used to select a  bucket, write/find your 
 * 		own or use one of xf_hash_*
 *
 * Example values for @size_bits
 *
 * 		1 - 2 buckets total (IDs 0-1)
 *
 * 		2 - 4 buckets total (IDs 0-3)
 *
 * 		3 - 8 buckets total (IDs 0-7)
 *
 * 		4 - 16 buckets total (IDs 0-15)
 */
XFFNC void xf_htable_construct(struct xf_htable *t, unsigned int size_bits,
		size_t value_size, uint32_t (*hash)(const char *,int));

/**
 * xf_htable_memcnt() - count dynamically allocated memory associated with htable
 * @t:		table which's memory to count
 *
 * Return:	amount of memory malloc()'ed
 */
XFFNC size_t xf_htable_memcnt(struct xf_htable *t);

/**
 * xf_htable_destruct() - releases all associated memory allocated to htable
 * @t:		the hashtable no longer required
 */
XFFNC void xf_htable_destruct(struct xf_htable *t);

/**
 * xf_htable_clear() - remove all entries but keep associated memory of htable
 * @t:		hashtable to reset
 */
XFFNC void xf_htable_clear(struct xf_htable *t);

/**
 * xf_htable_set() - add or replace a key/data combination in the hashtable
 * @t:		the table to put the value in
 * @key:	the key to associate the value with
 * @keylen:	length of @key in bytes
 * @value_in:	where to read the value associated with the key from -  
 * 		@t->value_size bytes will be read and copied
 *
 * Note that the key is not always copied!
 *
 * Return: 	%XF_HTABLE_ESUCCESS on success, %XF_HTABLE_EFULL if the bucket
 * 		can't store it 
 */
XFFNC int xf_htable_set(struct xf_htable *t, const void *key, size_t keylen, 
		const void *value_in);

/**
 * xf_htable_see() - see to that given key is in the table
 * @t:		hashtable to look in
 * @key:	key to associate value with
 * @keylen:	length of @key
 * @value_def:	value to associate the key with if it's not in the table or
 * 		%NULL to memset() the value to null
 *
 * Return:	%NULL if a new entry could not be added and entry is not in 
 * 		table
 */
XFFNC void *xf_htable_see(struct xf_htable *t, const void *key, size_t keylen,
		const void *value_def);

/**
 * xf_htable_find() - looks up the value for given key 
 * @t:		hashtable to look in
 * @key:	key to search for
 * @keylen:	length of the @key in bytes
 *
 * Return:	%NULL or a pointer to the value as it is in the bucket, feel 
 * 		free to modify
 */
XFFNC void *xf_htable_find(struct xf_htable *t, const void *key, size_t keylen);


/**
 * xf_htable_verify() - verify that a key/value pair exists in table
 * @t:		hashtable to look in
 * @key:	key to search for
 * @keylen:	length of @key
 * @value:	the value to match
 *
 * Return:	%XF_HTABLE_ESUCCESS if the key/value pair is in the table and 
 * 		equal
 *
 * 		%XF_HTABLE_ENOTFOUND if such key isn't in the table
 *
 * 		%XF_HTABLE_ENOTEQUAL if corresponding entry was found for @key
 * 		and that the values aren't equal (bear in mind that padding 
 * 		bytes in structures are undetermined unless specifically set 
 * 		with memset() or calloc())
 */
static inline int xf_htable_verify(struct xf_htable *t, const void *key, size_t keylen, 
		const void *value)
{
	void *v = xf_htable_find(t, key, keylen);
	if (v == NULL) return XF_HTABLE_ENOTFOUND;
	return XF_HTABLE_ESUCCESS; 
}

/**
 * xf_htable_remove() - looks up the given key and removes it with its value
 * @t:		hashtable to look in
 * @key:	key to search for
 * @keylen:	length of the @key in bytes
 *
 * Return:	%XF_HTABLE_ESUCCESS on success, %XF_HTABLE_ENOTFOUND if key 
 * 		wasn't found
 */
XFFNC int xf_htable_remove(struct xf_htable *t, const void *key,
		size_t keylen);


/**
 * DOC: xf_htable_entry* - for skipping multiple lookups
 * Useful when you wish to peek at the value before deciding whether or not to
 * further mess with it. ( set only if not found and whatnot )
 */
/**
 * struct xf_htable_entry - hashtable lookup information, possible entry
 * @key:		key to look for
 * @keylen:		length of @key
 *
 * Initialize @key and @keylen before using.
 */
struct xf_htable_entry {
	void *key;
	size_t keylen;
	/* private: the bucket the data is or would be in */
	unsigned int bucket_index;
	/* index of data inside the bucket, where it would be, or indetermined 
	 * if the bucket doesn't exist */
	unsigned short data_index;
};
/**
 * xf_htable_entry_find() - looks up given key
 * @t:		hashtable to look in
 * @e:		entry information, @key and @keylen should be set
 *
 * Return:	%XF_HTABLE_ESUCCESS if entry was found, %XF_HTABLE_ENOTFOUND if
 * 		entry was not found
 */
XFFNC int xf_htable_entry_find(struct xf_htable *t, struct xf_htable_entry *e);

/**
 * xf_htable_entry_val() - retrieve a value associated with entry
 * @t:		hashtable to look in
 * @e:		entry information for which xf_htable_entry_find() returned
 * 		%XF_HTABLE_ESUCCESS or where a value was just inserted
 */
XFFNC void *xf_htable_entry_val(struct xf_htable *t, 
		struct xf_htable_entry *e);

/**
 * xf_htable_entry_insert() - adds given entry and value to the table
 * @t:		hashtable to look in
 * @e:		entry information, xf_htable_entry_find() must have been called 
 * 		for given entry with same @key and @keylen values
 * @value_in:	value to associate with given entry
 *
 * Return:	%XF_HTABLE_ESUCCESS if insertion succeeded. 
 *
 * 		%XF_HTABLE_EFULL if bucket is full.
 *
 * 		%XF_HTABLE_ESET if the key was found already set to some value
 */
XFFNC int xf_htable_entry_insert(struct xf_htable *t, struct xf_htable_entry *e,
		void *value_in);
/**
 * xf_htable_entry_remove() - remove given key and associated value from table
 * @t:		hashtable to remove from
 * @e:		entry information, xf_htable_entry_find() must have been called
 * 		for given entry with the same @key and @keylen values
 *
 * Return:	%XF_HTABLE_ESUCCESS if removal succeeded.
 */
XFFNC int xf_htable_entry_remove(struct xf_htable *t, 
		struct xf_htable_entry *e);

/**
 * xf_hash_jenkins_oaat() - Bob Jenkins' One-at-a-Time hash
 * @key:	key to hash
 * @len:	length of the @key
 *
 * Source: http://www.burtleburtle.net/bob/hash/doobs.html
 */
XFFNC uint32_t xf_hash_jenkins_oaat(const char *key, int len);

/**
 * xf_hash_hsieh_superfast() - Paul Hsieh's SuperFastHash
 * @data:	key to hash
 * @len:	length of @data
 *
 * Source: http://www.azillionmonkeys.com/qed/hash.html
 */
XFFNC uint32_t xf_hash_hsieh_superfast(const char *data, int len);


#if XFSTATIC == 1 // Include function bodies?
#include "xf-htable.c"
#endif

#if !defined(_XF_MACROS) || _XF_MACROS == 0
// Clear up some internal «local» definitions
#undef XFFNC
#undef XFNOWRN
#undef XFSTATIC
#endif

#endif 
