/*
 * Summary: Batch request structure used for libmemcached.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Adam Thomason
 */

#ifndef __MEMCACHED_BATCH_H__
#define __MEMCACHED_BATCH_H__

#ifdef __cplusplus
extern "C" {
#endif

struct memcached_batch_st {
  uint32_t flags;
  bool is_allocated;

  size_t capacity;
  size_t number_of_keys;

  /* arrays of size `capacity`, of which `number_of_keys` have real data */
  const char **keys;
  size_t *key_lengths;
  uint32_t *key_hashes;

  memcached_st *root;
};

/* Batch Struct */
LIBMEMCACHED_API
memcached_batch_st *memcached_batch_create(memcached_st *ptr, 
                                           memcached_batch_st *batch);
LIBMEMCACHED_API
memcached_batch_st *memcached_batch_create_sized(memcached_st *memc,
                                           memcached_batch_st *ptr,
                                           size_t initial_size);
LIBMEMCACHED_API
void memcached_batch_free(memcached_batch_st *batch);
LIBMEMCACHED_API
void memcached_batch_reset(memcached_batch_st *ptr);

LIBMEMCACHED_API
void memcached_batch_get(memcached_batch_st *ptr,
                         const char* key, size_t key_length);
LIBMEMCACHED_API
void memcached_batch_get_by_key(memcached_batch_st *ptr,
                                const char* key, size_t key_length,
                                const char* master_key, size_t master_key_length);
LIBMEMCACHED_API
void memcached_batch_get_by_hash(memcached_batch_st *ptr,
                                 const char* key, size_t key_length,
                                 unsigned int hash);

#ifdef __cplusplus
}
#endif

#endif /* __MEMCACHED_BATCH_H__ */
