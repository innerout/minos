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
void *generate_populate_key(uint32_t *key_size)
{
	static uint32_t counter = COUNTER_START + 1;
	char buffer[12] = { 0 }; // Large enough for a 32-bit integer
	sprintf(buffer, "%u", counter);
	counter += 2;

	*key_size = strlen(buffer) + 1; // +1 for the null terminator
	log_debug("generated key size is %u", *key_size);
	char *key = calloc(1UL, *key_size);
	strcpy(key, buffer);
	log_debug("Created key: %s for insertion", key);

	return key;
}

void *generate_search_key(uint32_t *key_size)
{
	static uint32_t counter = COUNTER_START;
	char buffer[12] = { 0 }; // Large enough for a 32-bit integer
	sprintf(buffer, "%u", counter);
	counter += 2;

	*key_size = strlen(buffer) + 1; // +1 for the null terminator
	char *key = calloc(1UL, *key_size);
	strcpy(key, buffer);

	return key;
}

// Test function
void test_minos(int num_pairs)
{
	struct minos *skiplist = minos_init();

	// Insert random key-value pairs
	for (int i = 0; i < num_pairs; ++i) {
		struct minos_insert_request ins_req;
		ins_req.key = generate_populate_key(&ins_req.key_size);
		ins_req.key_size = strlen(ins_req.key);
		ins_req.value = ins_req.key;
		ins_req.value_size = ins_req.key_size;
		// ins_req.tombstone = 0;

		minos_insert(skiplist, &ins_req);

		free(ins_req.key);
	}

	// Test random keys with minos_seek_equal_or_less
	for (int i = 0; i < num_pairs + 1; ++i) {
		uint32_t search_key_size;
		char *search_key = generate_search_key(&search_key_size);
		log_debug("<seek no: %d>", i);
		struct minos_value result = minos_seek(skiplist, search_key_size, search_key);
		int ret = -1;
		if (result.found) {
			log_debug("Jackpot got exact match");
			free(search_key);
			continue;
		}
		log_debug("Result value size: %u pointer: %p", result.value_size, result.value);
		ret = memcmp(result.value, search_key,
			     search_key_size < result.value_size ? search_key_size : result.value_size);
		if (0 == ret)
			ret = result.value_size - search_key_size;
		if (ret > 0) {
			log_fatal("Failure! land on a key: %.*s that is equal or less of the search key: %.*s",
				  result.value_size, (char *)result.value, search_key_size, search_key);
			_exit(EXIT_FAILURE);
		}
		log_debug("Correct! land on a key: %.*s that is equal or less of the search key: %.*s",
			  result.value_size, (char *)result.value, search_key_size, search_key);

		log_debug("</seek>");
		free(search_key);
	}
	log_info("SUCCESS!");
	minos_free(skiplist);
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
