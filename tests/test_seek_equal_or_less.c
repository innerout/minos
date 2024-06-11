#include "minos.h"
#include <assert.h>
#include <log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define MAX_KEY_SIZE 256
#define MAX_VALUE_SIZE 512
#define COUNTER_START 1000000
#define POPULATE_STEP 2

void *generate_delete_key(uint32_t *key_size)
{
	static uint32_t delete_counter = COUNTER_START + 1;
	char buffer[12] = { 0 }; // Large enough for a 32-bit integer
	sprintf(buffer, "%u", delete_counter);
	delete_counter += POPULATE_STEP;

	*key_size = strlen(buffer) + 1; // +1 for the null terminator
	char *key = calloc(1UL, *key_size);
	strcpy(key, buffer);

	return key;
}

void *generate_populate_key(uint32_t *key_size)
{
	static uint32_t pop_counter = COUNTER_START + 1;
	char buffer[12] = { 0 }; // Large enough for a 32-bit integer
	sprintf(buffer, "%u", pop_counter);
	pop_counter += POPULATE_STEP;

	*key_size = strlen(buffer) + 1; // +1 for the null terminator
	char *key = calloc(1UL, *key_size);
	strcpy(key, buffer);

	return key;
}

void *generate_search_key(uint32_t *key_size)
{
	static uint32_t read_counter = COUNTER_START;
	char buffer[12] = { 0 }; // Large enough for a 32-bit integer
	sprintf(buffer, "%u", read_counter);
	read_counter += 1;

	*key_size = strlen(buffer) + 1; // +1 for the null terminator
	char *key = calloc(1UL, *key_size);
	strcpy(key, buffer);

	return key;
}

bool process_callback(void *value, void *cnxt)
{
	log_debug("Hello!");
	return true;
}
// Test function
void test_minos(int num_pairs)
{
	struct minos *skiplist = minos_init(true);
	fprintf(stderr, "Is minos empty?: %s\n", minos_is_empty(skiplist) ? "YES" : "NO");

	// Insert random key-value pair
	for (int i = 0; i < num_pairs; ++i) {
		struct minos_insert_request ins_req;
		ins_req.key = generate_populate_key(&ins_req.key_size);
		ins_req.value = ins_req.key;
		ins_req.value_size = ins_req.key_size;

		log_debug("Created key: %s for insertion of size %u", (char *)ins_req.key, ins_req.key_size);
		minos_insert(skiplist, &ins_req);

		free(ins_req.key);
	}
	fprintf(stderr, "Is minos empty?: %s\n", minos_is_empty(skiplist) ? "YES" : "NO");

	// Test random keys with minos_seek_equal_or_less
	// for (int i = 0; i < num_pairs; ++i) {
	// 	uint32_t search_key_size;
	// 	char *search_key = generate_search_key(&search_key_size);
	// 	log_debug("<seeking search key: %s of size %u>", search_key, search_key_size);
	// 	struct minos_iterator iter = { 0 };
	// 	bool exact_match = false;
	// 	bool valid =
	// 		minos_iter_seek_equal_or_imm_less(&iter, skiplist, search_key_size, search_key, &exact_match);
	// 	int ret = -1;
	// 	if (0 == i && valid) {
	// 		log_fatal("Oops valid? Search key: %*.s landed on key %.*s It shouldn't", search_key_size,
	// 			  search_key, iter.iter_node->kv->key_size, (char *)iter.iter_node->kv->key);
	// 		_exit(EXIT_FAILURE);
	// 	}
	// 	if (i & !valid) {
	// 		log_fatal("Oops no match for search key: %.*s? It shouldn't", search_key_size, search_key);
	// 		_exit(EXIT_FAILURE);
	// 	}
	// 	if (!valid)
	// 		continue;
	// 	ret = memcmp(search_key, iter.iter_node->kv->key,
	// 		     search_key_size < iter.iter_node->kv->key_size ? search_key_size :
	// 								      iter.iter_node->kv->key_size);
	// 	if (0 == ret)
	// 		ret = search_key_size - iter.iter_node->kv->key_size;
	// 	if (ret < 0) {
	// 		log_fatal("Iter found key %s that is less than search key %s", iter.iter_node->kv->key,
	// 			  search_key);
	// 		_exit(EXIT_FAILURE);
	// 	}
	// 	if (0 == ret) {
	// 		log_warn("Exact match nice!");
	// 	}
	// }

	//Now delete everything
	for (int i = 0; i < num_pairs; ++i) {
		uint32_t key_size;
		char *key = generate_delete_key(&key_size);
		log_debug("Created key: %s for deletion of size %u", (char *)key, key_size);
		if(false == minos_delete(skiplist, key, key_size)){
      log_fatal("Failed to delete key: %.*s",key_size,key);
      _exit(EXIT_FAILURE);
    }
		free(key);
	}
  if(false == minos_is_empty(skiplist)){
    log_fatal("Skip list should be empty after delete");
    _exit(EXIT_FAILURE);
  }
	fprintf(stderr, "Is minos empty?: %s\n", minos_is_empty(skiplist) ? "YES" : "NO");

	// Insert random key-value pair
	for (int i = 0; i < num_pairs; ++i) {
		struct minos_insert_request ins_req;
		ins_req.key = generate_populate_key(&ins_req.key_size);
		ins_req.value = ins_req.key;
		ins_req.value_size = ins_req.key_size;

		log_debug("<Repopulation> Created key: %s for insertion of size %u", (char *)ins_req.key,
			  ins_req.key_size);
		minos_insert(skiplist, &ins_req);

		free(ins_req.key);
	}
  log_info("Freeing list");
  minos_free(skiplist, process_callback, NULL);
	log_info("SUCCESS!");
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <num_pairs>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int num_pairs = atoi(argv[1]);
	if (num_pairs <= 0) {
		fprintf(stderr, "Number of key-value pairs must be positive\n");
		return EXIT_FAILURE;
	}

	srand(time(NULL)); // Seed the random number generator
	test_minos(num_pairs);

	return EXIT_SUCCESS;
}
