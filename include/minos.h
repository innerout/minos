#ifndef MINOS_H
#define MINOS_H
#ifdef __cplusplus
extern "C" {
#endif

#define SKPLIST_MAX_LEVELS 12 //this variable will be at conf file. It shows the max_levels of the skiplist
//it should be allocated according to L0 size
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
/* kv_category has the same format as Parallax
 * users can define the category of their keys
 * IMPORTANT: the *_INLOG choices should not be used for in-memory staff!
*/
// enum kv_category {
// 	SKPLIST_SMALL_INPLACE = 0,
// 	SKPLIST_SMALL_INLOG,
// 	SKPLIST_MEDIUM_INPLACE,
// 	SKPLIST_MEDIUM_INLOG,
// 	SKPLIST_BIG_INLOG,
// 	SKPLIST_UNKNOWN_LOG_CATEGORY,
// 	SKLIST_BIG_INPLACE
// };

enum kv_type { SKPLIST_KV_FORMAT = 19, SKPLIST_KV_PREFIX = 20 };

struct node_data {
	uint32_t key_size;
	uint32_t value_size;
	void *key;
	void *value;
};

struct minos_node {
	/*for parallax use*/
	struct minos_node *fwd_pointer[SKPLIST_MAX_LEVELS];
	pthread_rwlock_t rw_nodelock;
	struct node_data *kv;
	uint32_t level;
	uint8_t tombstone;
	uint8_t is_NIL;
};

struct minos_iterator {
	pthread_rwlock_t rw_iterlock;
	struct minos_node *iter_node;
	uint8_t is_valid;
};

struct minos_insert_request {
	uint32_t key_size;
	uint32_t value_size;
	void *key;
	void *value;
	uint8_t tombstone : 1;
};

struct minos {
	uint32_t level; //this variable will be used as the level hint
	struct minos_node *header;
	struct minos_node *NIL_element; //last element of the skip list
	/* a generic key comparator, comparator should return:
	 * > 0 if key1 > key2
	 * < 0 key2 > key1
	 * 0 if key1 == key2 */
	int (*comparator)(void *key1, void *key2, char key1_format, char key2_format);

	/* generic node allocator */
	struct minos_node *(*make_node)(struct minos_insert_request *ins_req);
};

struct minos_value {
	void *value;
	uint32_t value_size;
	uint8_t found;
};

struct minos *minos_init(void);
void minos_change_comparator(struct minos *skplist,
			     int (*comparator)(void *key1, void *key2, char key1_format, char key2_format));

void minos_change_node_allocator(struct minos *skplist,
				 struct minos_node *make_node(struct minos_insert_request *ins_req));
/*skiplist operations*/
struct minos_value minos_search(struct minos *skplist, uint32_t key_size, void *search_key);
void minos_insert(struct minos *skplist, struct minos_insert_request *ins_req);
bool minos_delete(struct minos *skplist, const char *key, uint32_t key_size); //TBI
void minos_free(struct minos *skplist);
/*iterators staff*/
void minos_iter_init(struct minos_iterator *iter, struct minos *skplist, uint32_t key_size, void *search_key);
void minos_iter_seek_first(struct minos_iterator *iter, struct minos *skplist);
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
