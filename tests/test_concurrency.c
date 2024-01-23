#include <assert.h>
#include <inttypes.h>
#include <minos.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KVS_NUM 1000000
#define KV_PREFIX "ts"
#define NUM_OF_THREADS 8

struct thread_info {
	pthread_t th;
	uint32_t *tid;
};
struct minos *concurrent_skiplist;

static void print_skplist(struct minos *skplist)
{
	struct minos_node *curr;
	for (int i = 0; i < SKIPLIST_MAX_LEVELS; i++) {
		curr = skplist->header;
		printf("keys at level %d -> ", i);
		while (!curr->is_NIL) {
			printf("[%s], ", (char *)curr->kv->key);
			curr = curr->fwd_pointer[i];
		}
		printf("\n");
	}
}

static void populate_skiplist_with_single_writer(struct minos *skplist)
{
	int i;
	struct minos_insert_request ins_req;
	char *key = malloc(strlen(KV_PREFIX) + sizeof(long long unsigned));
	uint32_t key_size;
	for (i = 0; i < KVS_NUM; i++) {
		memcpy(key, KV_PREFIX, strlen(KV_PREFIX));
		sprintf(key + strlen(KV_PREFIX), "%llu", (long long unsigned)i);
		key_size = strlen(key);
		ins_req.key_size = key_size;
		ins_req.key = key;
		ins_req.value = key;
		ins_req.value_size = key_size;
		// ins_req.tombstone = 0;
		minos_insert(skplist, &ins_req);
	}
	free(key);
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
	fprintf(stderr, "inserting from %d to %d\n", from, to);
	for (i = from; i < to; i++) {
		memcpy(key, KV_PREFIX, strlen(KV_PREFIX));
		sprintf(key + strlen(KV_PREFIX), "%llu", (long long unsigned)i);
		key_size = strlen(key);
		ins_req.key_size = key_size;
		ins_req.key = key;
		ins_req.value = key;
		ins_req.value_size = key_size;
		// ins_req.tombstone = 0;
		minos_insert(concurrent_skiplist, &ins_req);
		//print_skplist(&my_skiplist);
	}
	free(key);
	pthread_exit(NULL);
}

static void validate_number_of_KVS(struct minos *skplist)
{
	int count = 0;
	struct minos_node *curr = skplist->header;

	while (curr->fwd_pointer[0] != skplist->NIL_element) {
		curr = curr->fwd_pointer[0];
		++count;
	}
	printf("Count is %d should be %d is it ok? %s\n", count, KVS_NUM, count == KVS_NUM ? "YES" : "NO");
	assert(count == KVS_NUM); //-1 for the header node
}
static void print_each_level_size(struct minos *skplist)
{
	uint64_t count, i;
	struct minos_node *curr;
	for (i = 0; i < SKIPLIST_MAX_LEVELS; i++) {
		count = 0;
		curr = skplist->header;

		while (curr->is_NIL == 0) {
			count++;
			curr = curr->fwd_pointer[i];
		}
		printf("level's %" PRIu64 "size is %" PRIu64 "\n", i, count);
	}
}

/*compare the lists and check if all the nodes are present in the concurrent list
 *according to the single writer list (which is correct)
 *the function checks only the level0 of the skiplists where all keys reside*/
static void compare_the_lists(struct minos *clist, struct minos *swlist)
{
	int ret;
	struct minos_node *ccurr, *swcurr;
	ccurr = clist->header->fwd_pointer[0]; //skip the header, start from the first key
	swcurr = swlist->header->fwd_pointer[0]; //skip the header, start from the first key

	while (swcurr->fwd_pointer[0] != swlist->NIL_element) {
		int key_size = swcurr->kv->key_size > ccurr->kv->key_size ? swcurr->kv->key_size : ccurr->kv->key_size;
		ret = memcmp(swcurr->kv->key, ccurr->kv->key, key_size);
		if (ret == 0) { //all good step
			swcurr = swcurr->fwd_pointer[0];
			ccurr = ccurr->fwd_pointer[0];
		} else {
			printf("Found key %s over %s that doesnt exist at the concurrent skiplist\n", swcurr->kv->key,
			       ccurr->kv->key);
			break;
		}
	}
}

int main()
{
	srand(time(0));
	int i;
	struct thread_info thread_buf[NUM_OF_THREADS];
	struct minos *skiplist_single_writer = minos_init();
	concurrent_skiplist = minos_init();

	populate_skiplist_with_single_writer(skiplist_single_writer);

	for (i = 0; i < NUM_OF_THREADS; i++) {
		thread_buf[i].tid = (uint32_t *)malloc(sizeof(int));
		*thread_buf[i].tid = i;
		pthread_create(&thread_buf[i].th, NULL, populate_the_skiplist, thread_buf[i].tid);
	}

	for (i = 0; i < NUM_OF_THREADS; i++) {
		pthread_join(thread_buf[i].th, NULL);
		free(thread_buf[i].tid);
	}

	compare_the_lists(concurrent_skiplist, skiplist_single_writer);
	validate_number_of_KVS(concurrent_skiplist);

	minos_free(skiplist_single_writer, NULL, NULL);
	minos_free(concurrent_skiplist, NULL, NULL);
}
