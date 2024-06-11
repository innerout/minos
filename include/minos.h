#ifndef MINOS_H
#define MINOS_H
// #include <bits/pthreadtypes.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SKIPLIST_MAX_LEVELS 12 
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

struct minos_node_data {
	uint32_t key_size;
	uint32_t value_size;
	void *key;
	void *value;
};

typedef void *minos_lock_t;
struct minos_node {
	struct minos_node *fwd_pointer[SKIPLIST_MAX_LEVELS];
	pthread_rwlock_t rw_nodelock;
	minos_lock_t node_lock;
	struct minos_node_data *kv;
	uint32_t level;
	uint8_t is_NIL;
};

struct minos_insert_request {
	uint32_t key_size;
	uint32_t value_size;
	void *key;
	void *value;
};

typedef int (*minos_comparator)(void *key1, void *key2, uint32_t keysize1, uint32_t keysize2);
typedef struct minos_node *(*minos_make_node)(struct minos_insert_request *ins_req);

struct minos {
	uint32_t level; //this variable will be used as the level hint
	struct minos_node *header;
	struct minos_node *NIL_element; //last element of the skip list

	/* a generic key comparator, comparator should return:
	 * > 0 if key1 > key2
	 * < 0 key2 > key1
	 * 0 if key1 == key2 */
	minos_comparator comparator;
	/* generic node allocator */
	minos_make_node make_node;
  bool enable_locks;
};

struct minos_iterator {
	pthread_rwlock_t rw_iterlock;
	struct minos_node *iter_node;
	struct minos *skiplist;
	uint8_t is_valid;
};

struct minos_value {
	void *value;
	uint32_t value_size;
	uint8_t found;
};


struct minos *minos_init(bool is_thread_safe);
bool minos_is_empty(struct minos *minos);
void minos_change_comparator(struct minos *skiplist, minos_comparator comparator);

void minos_change_node_allocator(struct minos *skiplist,
				 struct minos_node *make_node(struct minos_insert_request *ins_req));
/*skiplist operations*/
struct minos_value minos_get_head_copy(struct minos *skiplist);
struct minos_value minos_search(struct minos *skiplist, uint32_t key_size, void *search_key);
struct minos_value minos_seek(struct minos *skiplist, uint32_t key_size, void *search_key);
void minos_insert(struct minos *skiplist, struct minos_insert_request *ins_req);
bool minos_delete(struct minos *skiplist, const char *key, uint32_t key_size); //TBI

typedef bool (*callback)(void *value, void *cnxt);
uint32_t minos_free(struct minos *skiplist, callback process, void *cnxt);
/*iterators staff*/
bool minos_iter_seek_equal_or_imm_less(struct minos_iterator *iter, struct minos *skiplist, uint32_t key_size,
				       char *search_key, bool *exact_match);
void minos_iter_seek_first(struct minos_iterator *iter, struct minos *skiplist);
void minos_iter_get_next(struct minos_iterator *iter);
char *minos_iter_get_key(struct minos_iterator *iter, uint32_t *key_size);
char *minos_iter_get_value(struct minos_iterator *iter, uint32_t *value_size);
/*return 1 if valid 0 if not valid*/
uint8_t minos_iter_is_valid(struct minos_iterator *iter);
void minos_iter_close(struct minos_iterator *iter);
#ifdef __cplusplus
}
#endif
#endif
