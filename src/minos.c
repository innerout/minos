#include <assert.h>
#include <limits.h>
#include <log.h>
#include <minos.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define DISABLE_LOCKS 0
/*FIXME this defines goes to parallax (already exists)*/
#define RWLOCK_INIT(L, attr) pthread_rwlock_init(L, attr)
#define RWLOCK_WRLOCK(L) pthread_rwlock_wrlock(L)
#define RWLOCK_RDLOCK(L) pthread_rwlock_rdlock(L)
#define RWLOCK_UNLOCK(L) pthread_rwlock_unlock(L)
/*FIXME this define goes to parallax(already exists)*/
#define MUTEX_INIT(L, attr) pthread_mutex_init(L, attr)
#define MUTEX_LOCK(L) pthread_mutex_lock(L)
#define MUTEX_UNLOCK(L) pthread_mutex_unlock(L)
#define MINOS_CACHE_LINE 64UL
pthread_mutex_t levels_lock_buf[SKIPLIST_MAX_LEVELS];

// static struct minos_lock_table *minos_init_lock_table(uint32_t size)
// {
// 	struct minos_lock_table *lock_table = calloc(1UL, sizeof(struct minos_lock_table));
// 	lock_table->locks = calloc(size, sizeof(struct minos_lock));
// 	lock_table->size = size;
// 	for (uint32_t i = 0; i < lock_table->size; i++)
// 		RWLOCK_INIT(&lock_table->locks[i].lock, NULL);
// 	return lock_table;
// }

// static void minos_destroy_lock_table(struct minos_lock_table *lock_table)
// {
// 	free(lock_table);
// }

static inline void minos_rd_lock(struct minos_node *node)
{
#if DISABLE_LOCKS
	return
#else
	RWLOCK_RDLOCK(&node->rw_nodelock);
#endif
}

static inline void minos_wr_lock(struct minos_node *node)
{
#if DISABLE_LOCKS
	return
#else
		RWLOCK_WRLOCK(&node->rw_nodelock);
#endif
}

static inline void minos_unlock(struct minos_node *node)
{
#if DISABLE_LOCKS
	return
#else
		RWLOCK_UNLOCK(&node->rw_nodelock);
#endif
}

//FIXME this should be static and removed from the test file
static uint32_t minos_random_level()
{
	uint32_t i;
	//SKPLIST_MAX_LEVELS - 1 cause we want a number from range [0,SKPLIST_MAX_LEVELS-1]
	for (i = 0; i < SKIPLIST_MAX_LEVELS - 1 && rand() % 4 == 0; i++)
		;

	return i;
}

static int minos_default_comparator(void *key1, void *key2, uint32_t keysize1, uint32_t keysize2)
{
	int ret = memcmp(key1, key2, keysize1 < keysize2 ? keysize1 : keysize2);
	// log_debug("Comparing key: %.*s with key %.*s result is %d keysize1: %u keysize2: %u\n", keysize1, (char *)key1,
	// 	  keysize2, (char *)key2, ret, keysize1, keysize2);
	return ret ? ret : keysize1 - keysize2;
}

static struct minos_node *minos_create_node(struct minos_insert_request *ins_req)
{
	struct minos_node *new_node = (struct minos_node *)calloc(1UL, sizeof(struct minos_node));
	new_node->kv = (struct minos_node_data *)calloc(1UL, sizeof(struct minos_node_data));

	/*create a node with the in-place kv*/
	new_node->kv->key = calloc(1UL, ins_req->key_size);
	new_node->kv->value = calloc(1UL, ins_req->value_size);
	new_node->kv->key_size = ins_req->key_size;
	new_node->kv->value_size = ins_req->value_size;
	memcpy(new_node->kv->key, ins_req->key, ins_req->key_size);
	memcpy(new_node->kv->value, ins_req->value, ins_req->value_size);
	new_node->is_NIL = 0;

	// LOCK TABLE RWLOCK_INIT(&new_node->rw_nodelock, NULL);
	new_node->node_lock = new_node;

	return new_node;
}

/*returns the biggest non-null level*/
static uint32_t minos_calc_level(struct minos *skplist)
{
	uint32_t i, lvl = 0;
	for (i = 0; i < SKIPLIST_MAX_LEVELS; i++) {
		if (skplist->header->fwd_pointer[i] != skplist->NIL_element)
			lvl = i;
		else
			break;
	}

	return lvl;
}

// skplist is an object called by reference
struct minos *minos_init(void)
{
	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	log_debug("There are %d cores in your system! size of pthread_rwlock_t is %lu", num_cores,
		  sizeof(pthread_rwlock_t));
#ifdef RELEASE_BUILD
	log_set_level(LOG_INFO);
#endif
	struct minos *skplist = (struct minos *)calloc(1UL, sizeof(struct minos));
	// allocate NIL (sentinel)
	skplist->NIL_element = (struct minos_node *)calloc(1UL, sizeof(struct minos_node));
	skplist->NIL_element->is_NIL = 1;
	skplist->NIL_element->level = 0;
	// LOCK TABLE if (RWLOCK_INIT(&skplist->NIL_element->rw_nodelock, NULL) != 0) {
	// 	exit(EXIT_FAILURE);
	// }
	// level is 0
	skplist->level = 0; //FIXME this will be the level hint

	skplist->header = (struct minos_node *)calloc(1UL, sizeof(struct minos_node));
	skplist->header->is_NIL = 0;
	skplist->header->level = 0;

	// if (RWLOCK_INIT(&skplist->header->rw_nodelock, NULL) != 0) {
	// 	exit(EXIT_FAILURE);
	// }

	// all forward pointers of header point to NIL
	for (int i = 0; i < SKIPLIST_MAX_LEVELS; i++)
		skplist->header->fwd_pointer[i] = skplist->NIL_element;

	skplist->comparator = minos_default_comparator;
	skplist->make_node = minos_create_node;

	return skplist;
}

void minos_change_comparator(struct minos *skplist, minos_comparator comparator)
{
	assert(skplist != NULL);
	skplist->comparator = comparator;
}

void minos_change_node_allocator(struct minos *skplist,
				 struct minos_node *make_node(struct minos_insert_request *ins_req))
{
	assert(skplist != NULL);
	skplist->make_node = make_node;
}

static struct minos_value minos_search_internal(struct minos *skiplist, uint32_t search_key_size, void *search_key,
						bool is_seek)
{
	int ret = INT_MAX;

	struct minos_node *curr, *next_curr;
	// RWLOCK_RDLOCK(&skiplist->header->rw_nodelock);
	minos_rd_lock(skiplist->header);

	curr = skiplist->header;
	//replace this with the hint level
	uint32_t levels;
	int level_id;
	levels = minos_calc_level(skiplist);

	for (level_id = levels; level_id >= 0; level_id--) {
		next_curr = curr->fwd_pointer[level_id];
		while (1) {
			if (curr->fwd_pointer[level_id]->is_NIL)
				break;
			// ret = memcmp(curr->fwd_pointer[i]->kv->key, search_key,
			// 	     curr->fwd_pointer[i]->kv->key_size < search_key_size ?
			// 		     curr->fwd_pointer[i]->kv->key_size :
			// 		     search_key_size);
			ret = skiplist->comparator(curr->fwd_pointer[level_id]->kv->key, search_key,
						   curr->fwd_pointer[level_id]->kv->key_size, search_key_size);
			if (ret < 0) {
				// RWLOCK_UNLOCK(&curr->rw_nodelock);
				minos_unlock(curr);
				curr = next_curr;
				// RWLOCK_RDLOCK(&curr->rw_nodelock);
				minos_rd_lock(curr);
				next_curr = curr->fwd_pointer[level_id];
			} else
				break;
		}
	}

	//we are infront of the node at level 0, node is locked
	//corner case
	//next element for level 0 is sentinel, key not found
	// ret = 1;
	if (!curr->fwd_pointer[0]->is_NIL) {
		// ret = memcmp(curr->fwd_pointer[0]->kv->key, search_key,
		// 	     curr->fwd_pointer[0]->kv->key_size < search_key_size ? curr->fwd_pointer[0]->kv->key_size :
		// 								    search_key_size);
		ret = skiplist->comparator(curr->fwd_pointer[0]->kv->key, search_key,
					   curr->fwd_pointer[0]->kv->key_size, search_key_size);
	}

	struct minos_value ret_val = { 0 };
	if (ret == 0 || is_seek) {
		ret_val.found = (ret == 0);
		if (is_seek && (curr->is_NIL || curr == skiplist->header)) {
			log_debug("Boom NIL");
			goto exit;
		}
		if (curr == skiplist->header)
			curr = curr->fwd_pointer[0];

		ret_val.value_size = curr->kv->value_size;
		ret_val.value = calloc(1UL, ret_val.value_size);
		memcpy(ret_val.value, curr->kv->value, ret_val.value_size);
		goto exit;
	}
exit:
	minos_unlock(curr);
	// RWLOCK_UNLOCK(&curr->rw_nodelock);
	return ret_val;
}

struct minos_value minos_search(struct minos *skiplist, uint32_t search_key_size, void *search_key)
{
	return minos_search_internal(skiplist, search_key_size, search_key, false);
}

struct minos_value minos_seek(struct minos *skiplist, uint32_t search_key_size, void *search_key)
{
	return minos_search_internal(skiplist, search_key_size, search_key, true);
}

/*(write)lock the node in front of node *key* at level lvl*/
static struct minos_node *getLock(struct minos *skiplist, struct minos_node *curr, struct minos_insert_request *ins_req,
				  int lvl)
{
	//see if we can advance further due to parallel modifications
	//first proceed with read locks, then acquire write locks
	/*Weak serach will be implemented later(performance issues)*/

	//...
	//
	int ret, node_key_size;
	struct minos_node *next_curr;

	if (lvl == 0) //if lvl is 0 we have locked the curr due to the search accross the levels
		minos_unlock(curr);

	//acquire the write locks from now on
	minos_wr_lock(curr);

	next_curr = curr->fwd_pointer[lvl];

	while (1) {
		if (curr->fwd_pointer[lvl]->is_NIL)
			break;

		ret = skiplist->comparator(curr->fwd_pointer[lvl]->kv->key, ins_req->key,
					   curr->fwd_pointer[lvl]->kv->key_size, ins_req->key_size);

		if (ret < 0) {
			minos_unlock(curr);
			curr = next_curr;
			minos_wr_lock(curr);
			next_curr = curr->fwd_pointer[lvl];
		} else
			break;
	}
	return curr;
}

void minos_insert(struct minos *skiplist, struct minos_insert_request *ins_req)
{
	int i, ret;
	uint32_t node_key_size, lvl;
	struct minos_node *update_vector[SKIPLIST_MAX_LEVELS];
	struct minos_node *curr, *next_curr;
	minos_rd_lock(skiplist->header);
	curr = skiplist->header;
	//we have the lock of the header, determine the lvl of the list
	lvl = minos_calc_level(skiplist);
	/*traverse the levels till 0 */
	for (i = lvl; i >= 0; i--) {
		next_curr = curr->fwd_pointer[i];
		while (1) {
			if (curr->fwd_pointer[i]->is_NIL) {
				break;
			}

			ret = skiplist->comparator(curr->fwd_pointer[i]->kv->key, ins_req->key,
						   curr->fwd_pointer[i]->kv->key_size, ins_req->key_size);

			if (ret < 0) {
				minos_unlock(curr);
				curr = next_curr;
				minos_rd_lock(curr);
				next_curr = curr->fwd_pointer[i];
			} else {
				break;
			}
		}
		update_vector[i] = curr; //store the work done until now, this may NOT be the final nodes
			//think that the concurrent inserts can update the list in the meanwhile
	}

	curr = getLock(skiplist, curr, ins_req, 0);
	//compare forward's key with the key
	if (!curr->fwd_pointer[0]->is_NIL)
		ret = skiplist->comparator(curr->fwd_pointer[0]->kv->key, ins_req->key,
					   curr->fwd_pointer[0]->kv->key_size, ins_req->key_size);
	else
		ret = 1;

	//updates are done only with the curr node write locked, so we dont have race using the
	//forward pointer
	if (ret == 0) { //update logic
		curr->fwd_pointer[0]->kv->value = calloc(1UL, ins_req->value_size);
		memcpy(curr->fwd_pointer[0]->kv->value, ins_req->value, ins_req->value_size);
		curr->fwd_pointer[0]->kv->value_size = ins_req->value_size;
		minos_unlock(curr);
		return;
	} else { //insert logic
		int new_node_lvl = minos_random_level();
		struct minos_node *new_node = skiplist->make_node(ins_req);
		new_node->level = new_node_lvl;
		//MUTEX_LOCK(&levels_lock_buf[new_node->level]); //needed for concurrent deletes

		//we need to update the header correcly cause new_node_lvl > lvl
		for (i = lvl + 1; i <= new_node_lvl; i++)
			update_vector[i] = skiplist->header;

		for (i = 0; i <= new_node->level; i++) {
			//update_vector might be altered, find the correct rightmost node if it has changed
			if (i != 0) {
				curr = getLock(skiplist, update_vector[i], ins_req,
					       i); //we can change curr now cause level i-1 has
			} //effectivly the new node and our job is done
			//linking logic
			new_node->fwd_pointer[i] = curr->fwd_pointer[i];
			curr->fwd_pointer[i] = new_node;
			minos_unlock(curr);
		}

		//MUTEX_UNLOCK(&levels_lock_buf[new_node->level]); //needed for concurrent deletes
	}
}

//update_vector is an array of size SKPLIST_MAX_LEVELS
static void minos_delete_key(struct minos *skplist, struct minos_node **update_vector, struct minos_node *curr)
{
	int i;
	for (i = 0; i <= skplist->level; i++) {
		if (update_vector[i]->fwd_pointer[i] != curr)
			break; //modifications for upper levels don't apply (passed the level of the curr node)

		update_vector[i]->fwd_pointer[i] = curr->fwd_pointer[i];
	}
	free(curr); //FIXME we will use tombstones?

	//previous skplist->level dont have nodes anymore. header points to NIL, so reduce the level of the list
	while (skplist->level > 0 && skplist->header->fwd_pointer[skplist->level]->is_NIL)
		--skplist->level;
}

bool minos_delete(struct minos *skiplist, const char *key, uint32_t key_size)
{
	int ret;
	struct minos_node *update_vector[SKIPLIST_MAX_LEVELS];
	struct minos_node *curr = skiplist->header;

	for (int i = skiplist->level; i >= 0; i--) {
		while (1) {
			if (curr->fwd_pointer[i]->is_NIL == 1)
				break; //reached sentinel

			// ret = memcmp(curr->fwd_pointer[i]->kv->key, key, key_size);
			ret = skiplist->comparator(curr->fwd_pointer[i]->kv->key, key,
						   curr->fwd_pointer[i]->kv->key_size, key_size);
			if (ret >= 0)
				break;
			curr = curr->fwd_pointer[i];
		}

		update_vector[i] = curr;
	}
	//retrieve it and check for existence
	curr = curr->fwd_pointer[0];
	if (!curr->is_NIL)
		// ret = memcmp(curr->kv->key, key, key_size);
		ret = skiplist->comparator(curr->kv->key, key, curr->kv->key_size, key_size);
	else
		return false; //Did not found the key to delete (reached sentinel)
	if (ret != 0)
		return false; //key not found
	minos_delete_key(skiplist, update_vector, curr);
	return true;
}

/*iterators staff*/
/*iterator is called by reference*/
/*iterator will hold the readlock of the corresponding search_key's node.
 ! all the inserts/update operations are valid except the ones containing that node(because for such modifications
 the write lock is needed)*/
void minos_iter_init(struct minos_iterator *iter, struct minos *skiplist, uint32_t search_key_size, void *search_key)
{
	int i, lvl;
	iter->skiplist = skiplist;
	struct minos_node *curr, *next_curr;
	int node_key_size, ret;
	minos_rd_lock(skiplist->header);

	curr = skiplist->header;
	//replace this with the hint level
	lvl = minos_calc_level(skiplist);

	for (i = lvl; i >= 0; i--) {
		next_curr = curr->fwd_pointer[i];
		while (1) {
			if (curr->fwd_pointer[i]->is_NIL) //reached sentinel for that level
				break;

			// node_key_size = curr->fwd_pointer[i]->kv->key_size;
			// if (node_key_size > key_size)
			// 	ret = memcmp(curr->fwd_pointer[i]->kv->key, search_key, node_key_size);
			// else
			// 	ret = memcmp(curr->fwd_pointer[i]->kv->key, search_key, key_size);
			ret = skiplist->comparator(curr->fwd_pointer[i]->kv->key, search_key,
						   curr->fwd_pointer[i]->kv->key_size, search_key_size);

			if (ret < 0) {
				minos_unlock(curr);
				curr = next_curr;
				minos_rd_lock(curr);
				next_curr = curr->fwd_pointer[i];
			} else
				break;
		}
	}
	//we are infront of the node at level 0, node is locked
	//corner case
	//next element for level 0 is sentinel, key not found
	if (!curr->fwd_pointer[0]->is_NIL) {
		node_key_size = curr->fwd_pointer[0]->kv->key_size;
		search_key_size = search_key_size > node_key_size ? search_key_size : node_key_size;
		// ret = memcmp(curr->fwd_pointer[0]->kv->key, search_key, search_key_size);
		ret = skiplist->comparator(curr->fwd_pointer[0]->kv->key, search_key,
					   curr->fwd_pointer[0]->kv->key_size, search_key_size);
	} else {
		printf("Reached end of the skiplist, didn't found key");
		iter->is_valid = 0;
		//RWLOCK_UNLOCK(&curr->rw_nodelock);
		minos_unlock(curr);
		return;
	}

	if (ret == 0) {
		iter->is_valid = 1;
		iter->iter_node = curr->fwd_pointer[0];
		/*lock iter_node unlock curr (remember curr is always behind the correct node)*/
		minos_rd_lock(iter->iter_node);
		// RWLOCK_UNLOCK(&curr->rw_nodelock);
		minos_unlock(curr);
	} else {
		printf("search key for scan init not found\n");
		iter->is_valid = 0;
		minos_unlock(curr);
	}
}

/*initialize a scanner to the first key of the skiplist
 *this is trivial, acquire the rdlock of the first level0 node */
void minos_iter_seek_first(struct minos_iterator *iter, struct minos *skiplist)
{
	iter->skiplist = skiplist;
	struct minos_node *curr, *next_curr;
	// RWLOCK_RDLOCK(&skplist->header->rw_nodelock);
	minos_rd_lock(skiplist->header);
	curr = skiplist->header;
	next_curr = curr->fwd_pointer[0];

	if (!curr->fwd_pointer[0]->is_NIL) {
		iter->is_valid = 1;
		iter->iter_node = curr->fwd_pointer[0];
		/*lock iter_node unlock curr (remember curr is always behind the correct node) */
		// RWLOCK_RDLOCK(&iter->iter_node->rw_nodelock);
		minos_rd_lock(iter->iter_node);
		// RWLOCK_UNLOCK(&curr->rw_nodelock);
	} else {
		log_debug("Reached end of skiplist, didn't found key");
		iter->is_valid = 0;
		minos_unlock(curr);
		return;
	}
}
/*we are searching level0 always so the next node is trivial to be found*/
void minos_iter_get_next(struct minos_iterator *iter)
{
	if (iter->is_valid == 1) {
		struct minos_node *next_node = iter->iter_node->fwd_pointer[0];
		if (next_node->is_NIL) {
			printf("Reached end of the skplist\n");
			iter->is_valid = 0;
			return;
		}
		//next_node is valid
		minos_unlock(iter->iter_node);
		iter->iter_node = next_node;
		minos_rd_lock(iter->iter_node);
	} else {
		log_fatal("iterator is invalid");
		_exit(EXIT_FAILURE);
	}
}

void minos_iter_close(struct minos_iterator *iter)
{
	minos_unlock(iter->iter_node);
	free(iter);
}

uint8_t minos_iter_is_valid(struct minos_iterator *iter)
{
	return iter->is_valid;
}

static inline bool minos_iter_check_iter_valid(struct minos_iterator *iter, uint32_t *size)
{
	return size && iter && minos_iter_is_valid(iter);
}

char *minos_iter_get_key(struct minos_iterator *iter, uint32_t *key_size)
{
	if (!minos_iter_check_iter_valid(iter, key_size)) {
		*key_size = 0;
		return NULL;
	}
	*key_size = iter->iter_node->kv->key_size;
	return iter->iter_node->kv->key;
}

char *minos_iter_get_value(struct minos_iterator *iter, uint32_t *value_size)
{
	if (!minos_iter_check_iter_valid(iter, value_size)) {
		*value_size = 0;
		return NULL;
	}
	*value_size = iter->iter_node->kv->value_size;
	return iter->iter_node->kv->value;
}

void minos_free(struct minos *skiplist)
{

	struct minos_node *curr = skiplist->header;

	while (!curr->is_NIL) {
		struct minos_node *next_curr = curr->fwd_pointer[0];
		// log_info("Freeing %lu", curr);
		if (curr->kv) {
			free(curr->kv->key);
			free(curr->kv->value);
			free(curr->kv);
		}
		free(curr);
		curr = next_curr;
	}
	//free sentinel
	if (curr->kv) {
		free(curr->kv->key);
		free(curr->kv->value);
		free(curr->kv);
	}
	free(curr);
	free(skiplist);
}

struct minos_value minos_get_head_copy(struct minos *skiplist)
{
	minos_rd_lock(skiplist->header);
	struct minos_value ret_val = {
		.value_size = skiplist->header->fwd_pointer[0]->kv->value_size,
	};
	if (0 == ret_val.value_size)
		goto exit;
	ret_val.value = calloc(1UL, ret_val.value_size);
	memcpy(ret_val.value, skiplist->header->fwd_pointer[0]->kv->value, ret_val.value_size);
	ret_val.found = 1;
exit:
	minos_unlock(skiplist->header);
	return ret_val;
}
