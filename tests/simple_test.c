#include <assert.h>
#include <inttypes.h>
#include <minos.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KVS_NUM 2000000
#define KV_PREFIX "ts"
#define NUM_OF_THREADS 7

struct thread_info {
	pthread_t th;
	uint32_t *tid;
};

struct minos *my_skiplist;

static void print_skplist(struct minos *skplist)
{
	struct minos_node *curr;
	for (int i = 0; i < SKIPLIST_MAX_LEVELS; i++) {
		curr = skplist->header;
		printf("keys at level %d -> ", i);
		while (!curr->is_NIL) {
			printf("[%d,%s], ", curr->kv->key_size, (char *)curr->kv->key);
			curr = curr->fwd_pointer[i];
		}
		printf("\n");
	}
}

static void *populate_the_skiplist(void *args)
{
	int i, from, to;
	char *key = malloc(strlen(KV_PREFIX) + sizeof(long long unsigned));
	int *tid = (int *)args;
	uint32_t key_size;
	struct minos_insert_request ins_req;
	from = (int)(((*tid) / (double)NUM_OF_THREADS) * KVS_NUM);
	to = (int)(((*tid + 1) / (double)NUM_OF_THREADS) * KVS_NUM);
	printf("inserting from %d to %d\n", from, to);
	for (i = from; i < to; i++) {
		memcpy(key, KV_PREFIX, strlen(KV_PREFIX));
		sprintf(key + strlen(KV_PREFIX), "%llu", (long long unsigned)i);
		key_size = strlen(key);
		ins_req.key_size = key_size;
		ins_req.key = key;
		ins_req.value_size = key_size;
		ins_req.value = key;
		ins_req.tombstone = 0;
		minos_insert(my_skiplist, &ins_req);
	}
	pthread_exit(NULL);
}

static void print_each_level_size(struct minos skplist)
{
	uint64_t count, i;
	struct minos_node *curr;
	for (i = 0; i < SKIPLIST_MAX_LEVELS; i++) {
		count = 0;
		curr = skplist.header;

		while (curr->is_NIL == 0) {
			count++;
			curr = curr->fwd_pointer[i];
		}
		printf("level's %" PRIu64 "size is %" PRIu64 "\n", i, count);
	}
}

static void delete_half_keys(struct minos *skplist)
{
	int i;
	char *key = malloc(strlen(KV_PREFIX) + sizeof(long long unsigned));

	for (i = 0; i < KVS_NUM / 2; i++) {
		memcpy(key, KV_PREFIX, strlen(KV_PREFIX));
		sprintf(key + strlen(KV_PREFIX), "%llu", (unsigned long long)i);
		printf("Deleting key%s\n", key);
		minos_delete(skplist, key, strlen(key));
		print_skplist(skplist);
	}
}

//this function also validates the return results
static void *search_the_skiplist(void *args)
{
	int i, from, to;
	char *key = malloc(strlen(KV_PREFIX) + sizeof(long long unsigned));
	int *tid = (int *)args;
	struct minos_value ret_val;
	uint32_t key_size;

	from = (int)(((*tid) / (double)NUM_OF_THREADS) * KVS_NUM);
	to = (int)(((*tid + 1) / (double)NUM_OF_THREADS) * KVS_NUM);
	printf("Searching from %d to %d\n", from, to);
	for (i = from; i < to; i++) {
		memcpy(key, KV_PREFIX, strlen(KV_PREFIX));
		sprintf(key + strlen(KV_PREFIX), "%llu", (unsigned long long)i);
		key_size = strlen(key);
		ret_val = minos_search(my_skiplist, key_size, key);
		assert(ret_val.found == 1);
		assert(memcmp(ret_val.value, key, ret_val.value_size) == 0); //keys and value are same in this test
	}
	pthread_exit(NULL);
}

static void validate_number_of_kvs()
{
	int count = 0;
	struct minos_node *curr = my_skiplist->header->fwd_pointer[0]; //skip the header

	while (!curr->is_NIL) {
		++count;
		curr = curr->fwd_pointer[0];
	}
	assert(count == KVS_NUM);
}

static void validate_number_of_kvs_with_iterators()
{
	int count = 0;
	struct minos_iterator *iter = (struct minos_iterator *)calloc(1, sizeof(struct minos_iterator));
	minos_iter_seek_first(iter, my_skiplist);
	while (minos_iter_is_valid(iter)) {
		++count;
		minos_iter_get_next(iter);
	}
	printf("Count is %d\n", count);
	assert(KVS_NUM == count);
	minos_iter_close(iter);
}

int main()
{
	srand(time(0));
	int i;
	struct thread_info thread_buf[NUM_OF_THREADS];

	my_skiplist = minos_init();
	assert(my_skiplist->level == 0);
	for (i = 0; i < SKIPLIST_MAX_LEVELS; i++)
		assert(my_skiplist->header->fwd_pointer[i] == my_skiplist->NIL_element);

	for (i = 0; i < NUM_OF_THREADS; i++) {
		thread_buf[i].tid = (uint32_t *)malloc(sizeof(int));
		*thread_buf[i].tid = i;
		pthread_create(&thread_buf[i].th, NULL, populate_the_skiplist, thread_buf[i].tid);
	}

	for (i = 0; i < NUM_OF_THREADS; i++)
		pthread_join(thread_buf[i].th, NULL);

	validate_number_of_kvs();
	printf("Validation of number of KVs passed\n");

	for (i = 0; i < NUM_OF_THREADS; i++) {
		thread_buf[i].tid = (uint32_t *)malloc(sizeof(int));
		*thread_buf[i].tid = i;
		pthread_create(&thread_buf[i].th, NULL, search_the_skiplist, thread_buf[i].tid);
	}

	for (i = 0; i < NUM_OF_THREADS; i++)
		pthread_join(thread_buf[i].th, NULL);

	validate_number_of_kvs_with_iterators();
	minos_free(my_skiplist);
}
