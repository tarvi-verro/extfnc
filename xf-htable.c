#if !defined(XFSTATIC) /* is .c processed first? */
#define _XF_STATIC 0 /* avoid looping between .c and .h */
#define _XF_MACROS 1
#include "xf-htable.h"
#endif

#include <stdlib.h> // realloc malloc
#include <string.h> // memset memcpy
#include <assert.h> // assert

#ifndef USHRT_MAX
#define USHRT_MAX ((unsigned short)~((unsigned short)0))
#endif

XFFNC void xf_htable_construct(struct xf_htable *t, unsigned int size_bits,
		size_t value_size, uint32_t (*hash)(const char*,int))
{
	assert(size_bits <= 32);
	t->res_mask = (1 << size_bits) - 1;
	t->hash = hash;
	t->value_size = value_size;
	t->buckets = calloc(1 << size_bits, sizeof(void *));
}

XFFNC void xf_htable_destruct(struct xf_htable *t)
{
	int i, l;
	for (i = 0, l = 1 + t->res_mask; i < l; i++) {
		if (t->buckets[i] == NULL) continue;
		free(t->buckets[i]);
	}
	free(t->buckets);
}

XFFNC void xf_htable_clear(struct xf_htable *t)
{
	int i, l;
	for (i = 0, l = 1 + t->res_mask; i < l; i++) {
		if (t->buckets[i] == NULL) continue;
		t->buckets[i]->length = 0;
	}
}

XFFNC size_t xf_htable_memcnt(struct xf_htable *t)
{
	assert(t->hash != NULL);
	assert(t->buckets != NULL);
	size_t cnt = 0;
	cnt += (t->res_mask + 1) * sizeof(void *);
	
	int i, l;
	for (i = 0, l = t->res_mask + 1; i < l; i++) {
		struct xf_htable_bucket *b = t->buckets[i];
		if (!b)
			continue;
		cnt += sizeof(struct xf_htable_bucket) 
			+ sizeof(union xf_htable_key) * b->size
			+ t->value_size * b->size;
	}
	return cnt;
}

/** 
 * xf_htable_get() - gets a slot for key/value pair
 * @t:		hashtable
 * @key:	key to get the slot for
 * @keylen:	length of @key
 * @b:		where to return a reference to a bucket
 * @pair_index:	where to write the index of the slot
 *
 * Return:	0 if XF_HTABLE_EFULL
 *
 * 		1 if given key was already in the table and it was returned
 *
 * 		2 if a new a new slot was allocated and returned
 */
static int xf_htable_get(struct xf_htable *t, const void *key, size_t keylen,
		struct xf_htable_bucket **rb, int *rpair_index)
{
	assert(t != NULL && key != NULL && rb != NULL && rpair_index != NULL);
	uint32_t bid = t->hash(key, keylen) & t->res_mask;
	struct xf_htable_bucket *b;
	if (t->buckets[bid] == NULL) {
		/* bucket capable of holding 1 pair */
		b = malloc(sizeof(struct xf_htable_bucket)
				+ 1 * sizeof(union xf_htable_key)
				+ 1 * t->value_size);
		b->size = 1;
		b->length = 0;
		t->buckets[bid] = b;
		goto newentry;
	} else {
		b = t->buckets[bid]; 
	}
	/* look for a matching key already in table */
	int i;
	for (i = 0; i < b->length; i++) {
		if (b->data[i].accesstyp == XF_HTABLE_KEY_DIRECT
				&& b->data[i].direct.length == keylen
				&& !memcmp(b->data[i].direct.a, key, keylen)) {
			*rpair_index = i;
			*rb = b;
			return 1;
		} else if (b->data[i].indirect.length == keylen
				&& !memcmp(b->data[i].indirect.ptr, key, keylen)) {  
			*rpair_index = i;
			*rb = b;
			return 1;
	       	}
	}
	/* no key in table - insert new*/
	if (b->size <= b->length) { /* expand bucket? */
		if (b->length + 1 > USHRT_MAX) {
			return 0;
		}
		int nsize = (XF_HTABLE_EXPANDFNC(b->size));
		nsize = nsize > USHRT_MAX ? USHRT_MAX : nsize;
		b = realloc(b, sizeof(struct xf_htable_bucket)
				+ sizeof(union xf_htable_key) * nsize 
				+ t->value_size + nsize);
		/* move values over */
		memmove(((char *) b->data) + nsize * sizeof(union xf_htable_key), 
				((char *) b->data) + 
				b->size * sizeof(union xf_htable_key), 
				b->length * t->value_size);
		b->size = nsize;
		t->buckets[bid] = b; /* lol */
	}


newentry:;
	int b_index = b->length;
	b->length++;
	union xf_htable_key *k = &b->data[b_index];
	if (keylen <= XF_HTABLE_KEY_DIRECT_MAX) {
		k->accesstyp = XF_HTABLE_KEY_DIRECT;
		k->direct.length = keylen;
		memcpy(k->direct.a, key, keylen);
	} else {
		k->accesstyp = XF_HTABLE_KEY_INDIRECT;
		k->indirect.length = keylen;
		k->indirect.ptr = key;
	}
	*rpair_index = b_index;
	*rb = b;
	return 2;
}

XFFNC int xf_htable_set(struct xf_htable *t, const void *key, size_t keylen,
		const void *value_in)
{
	struct xf_htable_bucket *b;
	int b_index;
	if (!xf_htable_get(t, key, keylen, &b, &b_index))
		return XF_HTABLE_EFULL;

	memcpy(xf_htable_bucket_val(t, b, b_index), value_in, 
			t->value_size);
	return XF_HTABLE_ESUCCESS;
}

XFFNC void *xf_htable_see(struct xf_htable *t, const void *key, size_t keylen,
		const void *value_def)
{ 
	struct xf_htable_bucket *b;
	int b_index;
	int gv = xf_htable_get(t, key, keylen, &b, &b_index);
	if (!gv) { 
		return NULL; 
	}
	void *val = xf_htable_bucket_val(t, b, b_index);
	if (gv == 2) {
		if (value_def == NULL)
			memset(val, 0, t->value_size);
		else
			memcpy(val, value_def, t->value_size);
	}
	return val;
}

XFFNC void *xf_htable_find(struct xf_htable *t, const void *key, size_t keylen)
{
	uint32_t bid = t->hash(key, keylen) & t->res_mask;
	struct xf_htable_bucket *b = t->buckets[bid];

	if (b == NULL) 
		return NULL;

	union xf_htable_key *k;
	int i;
	for (i = 0; i < b->length; i++) {
		k = b->data + i;
		if (k->accesstyp == XF_HTABLE_KEY_DIRECT 
				&& k->direct.length == keylen
				&& !memcmp(k->direct.a, key, keylen))
			return xf_htable_bucket_val(t, b, i);
		else if (k->accesstyp == XF_HTABLE_KEY_INDIRECT
				&& k->indirect.length == keylen
				&& !memcmp(k->indirect.ptr, key, keylen))
			return xf_htable_bucket_val(t, b, i);
	}
	return NULL;
}

XFFNC int xf_htable_remove(struct xf_htable *t, const void *key, 
		size_t keylen)
{
	assert(1!=1); // requires implementation
	return XF_HTABLE_ESUCCESS;
}

XFFNC int xf_htable_entry_find(struct xf_htable *t, struct xf_htable_entry *e)
{
	assert(1==2); // requires implementation
	return XF_HTABLE_ESUCCESS;
}

XFFNC void *xf_htable_entry_val(struct xf_htable *t, struct xf_htable_entry *e)
{
	assert(1==2); // requires implementation
	return NULL;
}

XFFNC int xf_htable_entry_insert(struct xf_htable *t, struct xf_htable_entry *e,
		void *value_in)
{
	assert(1==2);
	return XF_HTABLE_ESUCCESS;
}

XFFNC int xf_htable_entry_remove(struct xf_htable *t, 
		struct xf_htable_entry *e)
{
	assert(1==2);
	return XF_HTABLE_ESUCCESS;
}

/**
 * xf_hash_jenkins_oaat() - Bob Jenkins' One-at-a-Time hash
 * @key:	key to hash
 * @len:	length of the @key
 *
 * Source: http://www.burtleburtle.net/bob/hash/doobs.html
 */
XFFNC uint32_t xf_hash_jenkins_oaat(const char* key, int len)
{
	uint32_t hash;
	size_t i;
	for(hash=0, i=0; i<len; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return (hash);
}

/**
 * xf_hash_hsieh_superfast() - Paul Hsieh's SuperFastHash
 * @data:	key to hash
 * @len:	length of @data
 *
 * Source: http://www.azillionmonkeys.com/qed/hash.html
 */
XFFNC uint32_t xf_hash_hsieh_superfast(const char *data, int len) 
{
#ifndef get16bits
# if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
	  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#  define get16bits(d) (*((const uint16_t *) (d)))
# else
#  define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
		                       +(uint32_t)(((const uint8_t *)(d))[0]) )
# endif
#endif
	uint32_t hash = len, tmp;
	int rem;

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
			hash += hash >> 11;
			break;
		case 2: hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1: hash += (signed char)*data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

