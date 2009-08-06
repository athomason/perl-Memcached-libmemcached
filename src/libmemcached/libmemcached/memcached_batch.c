/*
  memcached_batch_st are used to internally represent a set of requested keys
  which can be built up before initiating a multi-get.
*/
#include "common.h"

memcached_batch_st *memcached_batch_create(memcached_st *memc,
                                           memcached_batch_st *ptr)
{
  return memcached_batch_create_sized(memc, ptr, 1);
}

memcached_batch_st *memcached_batch_create_sized(memcached_st *memc,
                                           memcached_batch_st *ptr,
                                           size_t initial_size)
{
  /* Saving malloc calls :) */
  if (ptr)
    memset(ptr, 0, sizeof(memcached_batch_st));
  else
  {
    ptr= memc->call_malloc(memc, sizeof(memcached_batch_st));

    if (ptr == NULL)
      return NULL;
    ptr->is_allocated= true;
  }

  ptr->root= memc;
  ptr->number_of_keys= 0;
  ptr->capacity= initial_size ? initial_size : 1; /* allocate at least one */
  ptr->keys= memc->call_malloc(memc, ptr->capacity * sizeof(char*));
  ptr->key_lengths= memc->call_malloc(memc, ptr->capacity * sizeof(size_t));
  ptr->key_hashes= memc->call_malloc(memc, ptr->capacity * sizeof(uint32_t));

  return ptr;
}

void memcached_batch_get(memcached_batch_st *ptr,
                         const char* key, size_t key_length)
{
  memcached_batch_get_by_hash(ptr, key, key_length,
                              memcached_generate_hash(ptr->root, key, key_length));
}

void memcached_batch_get_by_key(memcached_batch_st *ptr,
                                const char* key, size_t key_length,
                                const char* master_key, size_t master_key_length)
{
  memcached_batch_get_by_hash(ptr, key, key_length,
                              memcached_generate_hash(ptr->root, master_key, master_key_length));
}

void memcached_batch_get_by_hash(memcached_batch_st *ptr,
                                 const char* key, size_t key_length,
                                 uint32_t hash)
{
  if (ptr->number_of_keys + 1 > ptr->capacity) {
    ptr->capacity *= 2;
    ptr->keys= ptr->root->call_realloc(ptr->root, ptr->keys, ptr->capacity * sizeof(char*));
    ptr->key_lengths= ptr->root->call_realloc(ptr->root, ptr->key_lengths, ptr->capacity * sizeof(size_t));
    ptr->key_hashes= ptr->root->call_realloc(ptr->root, ptr->key_hashes, ptr->capacity * sizeof(uint32_t));
  }

  ptr->keys[ptr->number_of_keys] = strndup(key, key_length);
  ptr->key_lengths[ptr->number_of_keys] = key_length;
  ptr->key_hashes[ptr->number_of_keys] = hash;

  ptr->number_of_keys++;
}

void memcached_batch_reset(memcached_batch_st *ptr)
{
  ptr->flags= 0;
  ptr->number_of_keys= 0;
  /* zero memory for other structures but leave allocated */
  memset(ptr->keys, 0, ptr->capacity*sizeof(char*));
  memset(ptr->key_lengths, 0, ptr->capacity*sizeof(size_t));
  memset(ptr->key_hashes, 0, ptr->capacity*sizeof(uint32_t));
}

void memcached_batch_free(memcached_batch_st *ptr)
{
  int i;
  if (ptr == NULL)
    return;

  for (i = 0; i < ptr->number_of_keys; i++)
    free(ptr->keys[i]); /* strndup uses system malloc */
  ptr->root->call_free(ptr->root, ptr->keys);
  ptr->root->call_free(ptr->root, ptr->key_lengths);
  ptr->root->call_free(ptr->root, ptr->key_hashes);

  if (ptr->is_allocated)
    free(ptr);
}
