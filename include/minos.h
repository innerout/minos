#ifndef MINOS_H
#define MINOS_H
// #include <bits/pthreadtypes.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SKIPLIST_MAX_LEVELS 12 //this variable will be at conf file. It shows the max_levels of the skiplist
//it should be allocated according to L0 size
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
/* kv_category has the same format as Parallax
 * users can define the category of their keys
 * IMPORTANT: the *_INLOG choices should not be used for in-memory staff!
*/
// enum kv_category {
// 	skiplist_SMALL_INPLACE = 0,
// 	skiplist_SMALL_INLOG,
// 	skiplist_MEDIUM_INPLACE,
// 	skiplist_MEDIUM_INLOG,
// 	skiplist_BIG_INLOG,
// 	skiplist_UNKNOWN_LOG_CATEGORY,
// 	SKLIST_BIG_INPLACE
// };

struct minos_lock {
	pthread_rwlock_t lock;
	char pad[8];
};

struct minos_lock_table {
	uint32_t size;
	struct minos_lock *locks;
};

struct minos_node_data {
	uint32_t key_size;
	uint32_t value_size;
	void *key;
	void *value;
};

typedef void *minos_lock_t;
struct minos_node {
	struct minos_node *fwd_pointer[SKIPLIST_MAX_LEVELS];
	// pthread_rwlock_t rw_nodelock;
	minos_lock_t node_lock;
	struct minos_node_data *kv;
	uint32_t level;
	// uint8_t tombstone;
	uint8_t is_NIL;
};

struct minos_insert_request {
	uint32_t key_size;
	uint32_t value_size;
	void *key;
	void *value;
	// uint8_t tombstone : 1;
};

typedef int (*minos_comparator)(void *key1, void *key2, uint32_t keysize1, uint32_t keysize2);
typedef struct minos_node *(*minos_make_node)(struct minos_insert_request *ins_req);

struct minos {
	uint32_t level; //this variable will be used as the level hint
	struct minos_node *header;
	struct minos_node *NIL_element; //last element of the skip list
	struct minos_lock_table *level_locks[SKIPLIST_MAX_LEVELS];

	/* a generic key comparator, comparator should return:
	 * > 0 if key1 > key2
	 * < 0 key2 > key1
	 * 0 if key1 == key2 */
	minos_comparator comparator;
	/* generic node allocator */
	minos_make_node make_node;
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

struct minos *minos_init(void);
void minos_change_comparator(struct minos *skiplist, minos_comparator comparator);

void minos_change_node_allocator(struct minos *skiplist,
				 struct minos_node *make_node(struct minos_insert_request *ins_req));
/*skiplist operations*/
struct minos_value minos_get_head_copy(struct minos *skiplist);
struct minos_value minos_search(struct minos *skiplist, uint32_t key_size, void *search_key);
struct minos_value minos_seek(struct minos *skiplist, uint32_t key_size, void *search_key);
void minos_insert(struct minos *skiplist, struct minos_insert_request *ins_req);
bool minos_delete(struct minos *skiplist, const char *key, uint32_t key_size); //TBI
void minos_free(struct minos *skiplist);
/*iterators staff*/
void minos_iter_init(struct minos_iterator *iter, struct minos *skiplist, uint32_t key_size, void *search_key);
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
