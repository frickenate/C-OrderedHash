#ifndef OHASH_
#define OHASH_

#include <inttypes.h> // int*_t, uint*_t
#include <sodium.h> // crypto_shorthash_KEYBYTES
#include <stdlib.h> // size_t

/**
 * INTERNAL - Singly linked list node, for memory allocations internal to hash instance.
 */
typedef struct OHashAllocation {
    void *allocation;
    void *next;
} OHashAllocation;

/**
 * INTERNAL - Doubly linked list node, for pair.
 */
typedef struct OHashPair {
    void *key;
    void *value;
    struct OHashPair *prev;
    struct OHashPair *next;
} OHashPair;

/**
 * INTERNAL - Singly linked list node, for referencing OHashPair instance.
 */
typedef struct OHashPairRef {
    OHashPair *pair;
    struct OHashPairRef *next;
} OHashPairRef;


/**
 * Options used to initialize new hash instances via ohash_new().
 */
typedef struct OHashOptions {
    /**
     * The function used to hash all keys.
     *
     * Takes OHashOptions and key as arguments; must return a numeric hash as uintmax_t.
     *
     * Custom functions can be used. For convenience, the following functions are bundled:
     *
     * * For pointer addresses of keys, disregarding data type:
     *  - ohash_key_pointer
     *
     * * For strings:
     *  - ohash_key_string
     *
     * * For signed integers:
     *  - ohash_key_int
     *  - ohash_key_intmax
     *  - ohash_key_int8
     *  - ohash_key_int16
     *  - ohash_key_int32
     *  - ohash_key_int64
     *
     * * For unsigned integers:
     *  - ohash_key_uint
     *  - ohash_key_uintmax
     *  - ohash_key_uint8
     *  - ohash_key_uint16
     *  - ohash_key_uint32
     *  - ohash_key_uint64
     *
     * * For floats/doubles:
     *  - ohash_key_float
     *  - ohash_key_double
     *  - ohash_key_long_double
     */
    uintmax_t(*key_hash_fn)(const struct OHashOptions, const void*);

    /**
     * The function used to compare keys for equality.
     *
     * Takes two keys as arguments; must return 1 if keys are equal, otherwise 0.
     *
     * Custom functions can be used. For convenience, the following functions
     * are bundled, designed to be used with the corresponding key_hash_fn:
     *
     * * For direct pointer comparison:
     *  - ohash_compare_key_pointer
     *
     * * For strings:
     *  - ohash_compare_key_string
     *
     * * For signed integers:
     *  - ohash_compare_key_int
     *  - ohash_compare_key_intmax
     *  - ohash_compare_key_int8
     *  - ohash_compare_key_int16
     *  - ohash_compare_key_int32
     *  - ohash_compare_key_int64
     *
     * * For unsigned integers:
     *  - ohash_compare_key_uint
     *  - ohash_compare_key_uintmax
     *  - ohash_compare_key_uint8
     *  - ohash_compare_key_uint16
     *  - ohash_compare_key_uint32
     *  - ohash_compare_key_uint64
     *
     * For floats/doubles:
     *  - ohash_compare_key_float
     *  - ohash_compare_key_double
     *  - ohash_compare_key_long_double
     */
    _Bool(*key_compare_fn)(const void*, const void*);

    /**
     * Function used to free memory of keys when they are removed from the hash.
     *
     * Takes key pointer as argument. No return value.
     *
     * Setting this option permits the hash to automatically free the memory
     * used by keys when they are removed from the hash via ohash_delete(),
     * ohash_delete_all() and ohash_free(), as well as when ohash_insert() replaces
     * a duplicate entry. Can simply be free(), a custom free()-like function,
     * or NULL to disable automatic freeing of memory associated with keys.
     */
    void(*key_free_fn)(void*);

    /**
     * Function used to free memory of values when they are removed from the hash.
     *
     * Takes value pointer as argument. No return value.
     *
     * Setting this option permits the hash to automatically free the memory
     * used by values when they are removed from the hash via ohash_delete(),
     * ohash_delete_all() and ohash_free(), as well as when ohash_insert() replaces
     * a duplicate entry. Can simply be free(), a custom free()-like function,
     * or NULL to disable automatic freeing of memory associated with values.
     */
    void(*value_free_fn)(void*);

    /**
     * Initial number of items for which to preallocate resources.
     *
     * This is merely a hint regarding the minimum number of total items the hash
     * should expect to hold. Defaults to 2. Regardless of the value set, hashes
     * automatically grow by a factor of 2 whenever more room is required. Thus,
     * setting this option is merely an optimizaton to reduce the total number
     * of memory allocations and iterations of resizing the hash table to grow, if
     * you know in advance you will be inserting a very large number of items.
     */
    size_t num_items;

    /**
     * Hash table bucket occupancy, as a percentage, at which the hash table grows larger.
     *
     * Value is a percentage between 0 and 100, defaulting to 75. A value
     * between 60 and 85 is recommended. Values below 60 are likely to waste
     * memory and cpu as the table resizes frequently with little benefit.
     * Values above 85 are likely to significantly affect performance, as key
     * collisions bloat the number of items each bucket must accommodate.
     */
    int resize_capacity_percent;

    /**
     * Secret key used as part of key hashing.
     *
     * All bundled hash functions (including for pointers, strings, and
     * signed and unsigned integers) use SipHash for hashing of keys. A
     * secret key is used to prevent denial-of-service attacks, whereby an
     * end user purposely crafts keys that cause hash collisions. This option
     * should be considered internal and never provided explicitly, as each
     * hash instance will generate a random secret of the correct length.
     */
    unsigned char hash_string_secret[crypto_shorthash_KEYBYTES];
} OHashOptions;

/**
 * Primary struct, representing a single hash instance.
 */
typedef struct OHash {
    /**
     * Number of pairs allocated, whether or not currently in use.
     */
    size_t num_pairs_allocated;

    /**
     * Number of pairs currently stored in hash.
     */
    size_t num_pairs_used;

    /**
     * Head of doubly linked list of pairs currently in use.
     */
    OHashPair *pairs_used_head;

    /**
     * Tail of pairs_used_head list; used to append pairs in insertion order.
     */
    OHashPair *pairs_used_tail;

    /**
     * Head of singly linked list of pairs allocated but not currently in use.
     *
     * While struct is doubly linked, prev is always NULL when pairs are in this list.
     */
    OHashPair *pairs_unused_head;

    /**
     * Number of pair refs allocated, whether or not currently in use.
     */
    size_t num_pair_refs_allocated;

    /**
     * Head of singly linked list of pair refs allocated but not currently in use.
     */
    OHashPairRef *pair_refs_unused_head;

    /**
     * Singly linked list of pair refs tracking pairs currently in zombie state.
     *
     * Each zombie is a pair ref to a pair still present in pairs_used_head list.
     * Zombies are only created when pairs are deleted while any iterators
     * exist, allowing all operations to be used safely during iteration.
     */
    OHashPairRef *pair_refs_zombie_head;

    /**
     * Number of allocated hash table buckets, whether or not occupied.
     */
    size_t num_buckets_allocated;

    /**
     * Number of hash table buckets currently occupied by one or more pair refs.
     */
    size_t num_buckets_occupied;

    /**
     * Allocated hash table buckets.
     *
     * Each bucket is head of singly linked list of pair refs, operating as
     * separate chaining mechanism for handling hash key bucket collisions.
     */
    OHashPairRef **buckets;

    /**
     * Options used to configure hash behavior, as passed to ohash_new().
     */
    OHashOptions options;

    /**
     * Doubly linked list of allocated iterators.
     */
    struct OHashIter *iterators;

    /**
     * Long-term memory allocations internal to hash, freed upon ohash_free().
     */
    OHashAllocation *allocations;
} OHash;

/**
 * Iterator used to iterate, in insertion order, a hash instance's key/value pairs.
 */
typedef struct OHashIter {
    _Bool free_on_destroy;  ///< whether iterator itself should be freed during hash destruction
    int first;              ///< whether we are at start of iteration (0/1)
    OHash *hash;            ///< hash instance the iterator instance belongs to
    OHashPair *pair;        ///< current pair iterator is pointing at
    struct OHashIter *prev; ///< prev iterator
    struct OHashIter *next; ///< next iterator
} OHashIter;

/**
 * Initializes hash instance allocated manually by the user.
 *
 * When finished with hash instance, a call to ohash_destroy() frees all associated internal resources.
 *
 * @param[in,out] hash Existing uninitialized hash instance.
 * @param[in] options List of options with which to initialize hash.
 * @return _Bool Whether initilization was successful and hash instance is usable.
 */
_Bool ohash_init(OHash *hash, OHashOptions options);

/**
 * Allocates and initializes new hash instance.
 *
 * When finished with hash instance, a call to ohash_free() frees all associated resources.
 *
 * @param[in] options List of options with which to initialize hash.
 * @retval OHash* If hash instance is successfully initialized.
 * @retval NULL If hash instance cannot be created, typically due to invalid options.
 */
OHash *ohash_new(OHashOptions options);

/**
 * Retrieves number of items currently stored in given hash instance.
 *
 * @param[in] hash An existing hash instance.
 * @return Number of items currently stored in hash.
 */
size_t ohash_count(const OHash *hash);

/**
 * Retrieves value stored under given key within given hash instance.
 *
 * @param[in] hash An existing hash instance.
 * @param[in] key Key to retrieve value for, as a null pointer which configured hash function can handle.
 * @retval void* If key is found, value stored under key as a null pointer.
 * @retval NULL If key is not found.
 */
void *ohash_get(const OHash *hash, const void *key);

/**
 * Determines whether given key is stored within given hash instance.
 *
 * @param[in] hash An existing hash instance.
 * @param[in] key Key to check existence of, as a null pointer which configured hash function can handle.
 * @retval 0 If key could not be found in hash.
 * @retval 1 If key was found in hash.
 */
_Bool ohash_exists(const OHash *hash, const void *key);

/**
 * Inserts given value under given key within given hash instance.
 *
 * If given key already exists within given hash instance, existing key and value
 * are both evicted - and freed if key and/or value freeing functions were given in
 * hash's options. The "fresh" key and value then replace what was previously stored.
 *
 * @param[in] hash An existing hash instance.
 * @param[in] key Key to insert, as a null pointer which configured hash function can handle.
 * @param[in] value Value to insert, as any valid null pointer.
 * @retval 0 If key/value pair could not be inserted.
 * @retval 1 If key/value pair was successfully inserted.
 */
_Bool ohash_insert(OHash *hash, void *key, void *value);

/**
 * Moves given existing key to first position (head) of given hash instance.
 *
 * @param[in] hash An existing hash instance.
 * @param[in] key A previously stored key, to be moved to head.
 * @return _Bool Whether key was found and moved to head (or is already head).
 */
_Bool ohash_move_key_head(OHash *hash, void *key);

/**
 * Moves given existing key to last position (tail) of given hash instance.
 *
 * @param[in] hash An existing hash instance.
 * @param[in] key A previously stored key, to be moved to tail.
 * @return _Bool Whether key was found and moved to tail (or is already tail).
 */
_Bool ohash_move_key_tail(OHash *hash, void *key);

/**
 * Deletes given key (and its stored value) from given hash instance.
 *
 * Key and/or value freeing functions given in hash's options are called.
 *
 * @param[in] hash An existing hash instance.
 * @param[in] key Key to delete, as a null pointer which configured hash function can handle.
 * @retval 0 If key/value pair could not be found or deleted.
 * @retval 1 If key/value pair was found and successfully deleted.
 */
_Bool ohash_delete(OHash *hash, void *key);

/**
 * Deletes all keys (and their stored values) from given hash instance.
 *
 * Key and/or value freeing functions given in hash's options are called for each key and value.
 *
 * @param hash An existing hash instance.
 * @retval 0 If any key/value pairs could not be deleted.
 * @retval 1 If all key/value pairs were successfully deleted - including if hash was already empty.
 */
_Bool ohash_delete_all(OHash *hash);

/**
 * Frees given hash instance's internal resources.
 *
 * 1. All iterators which have not been freed via ohash_iter_free() are forcibly freed.
 * 2. All stored keys are values are deleted (and freed if hash options specified freeing functions).
 * 3. All allocations internal to hash are freed.
 * 4. Hash instance itself IS NOT freed.
 *
 * NOTE: Once called, hash instance (including iterators) is invalid unless reinitialized.
 *
 * @param[in] hash
 */
void ohash_destroy(OHash *hash);

/**
 * Frees given hash instance in its entirety.
 *
 * 1. All iterators which have not been freed via ohash_iter_free() are forcibly freed.
 * 2. All stored keys are values are deleted (and freed if hash options specified freeing functions).
 * 3. All allocations internal to hash are freed.
 * 4. Hash instance itself IS freed.
 *
 * NOTE: Once called, all pointers to given hash instance (including iterators) are invalid.
 *
 * @param[in] hash
 */
void ohash_free(OHash *hash);

/**
 * Initializes iterator instance allocated manually by the user.
 *
 * When finished with iterator instance, a call to ohash_iter_destroy() frees all associated internal resources.
 *
 * @param[in,out] iterator Existing uninitialized iterator instance.
 * @param[in] hash An existing, initialized, hash instance.
 * @return _Bool Whether initilization was successful and iterator instance is usable.
 */
void ohash_iter_init(OHashIter *iterator, OHash *hash);

/**
 * Allocates and initializes new iterator instance for given hash instance.
 *
 * @param[in] hash An existing hash instance.
 * @return New iterator instance.
 */
OHashIter *ohash_iter_new(OHash *hash);

/**
 * Rewinds iterator, allowing iteration to begin from start.
 *
 * @param[in] iterator An existing iterator instance.
 */
void ohash_iter_rewind(OHashIter *iterator);

/**
 * Advance iterator to next key/value pair.
 *
 * @param[in] iterator An existing iterator instance.
 * @retval 0 If iteration is complete; ie: there are no more key/value pairs.
 * @retval 1 If iterator has successfully advanced to next key/value pair.
 */
_Bool ohash_iter_each(OHashIter *iterator);

/**
 * Retrieves key for current iteration.
 *
 * @param[in] iterator An existing iterator instance.
 * @retval void* Key for current iteration if iterator position is valid.
 * @retval NULL If iterator position is invalid.
 */
void *ohash_iter_key(const OHashIter *iterator);

/**
 * Retrieves value for current iteration.
 *
 * @param[in] iterator An existing iterator instance.
 * @retval void* Value for current iteration if iterator position is valid.
 * @retval NULL If iterator position is invalid.
 */
void *ohash_iter_value(const OHashIter *iterator);

/**
 * Frees given iterator instance's internal resources.
 *
 * @param iterator[in] An existing iterator instance.
 */
void ohash_iter_destroy(OHashIter *iterator);

/**
 * Frees given iterator instance in its entirety.
 *
 * @param iterator[in] An existing iterator instance.
 */
void ohash_iter_free(OHashIter *iterator);


/* bundled key comparison functions */

_Bool ohash_compare_key_pointer(const void *a, const void *b);
_Bool ohash_compare_key_string(const void *a, const void *b);
_Bool ohash_compare_key_int(const void *a, const void *b);
_Bool ohash_compare_key_intmax(const void *a, const void *b);
_Bool ohash_compare_key_int8(const void *a, const void *b);
_Bool ohash_compare_key_int16(const void *a, const void *b);
_Bool ohash_compare_key_int32(const void *a, const void *b);
_Bool ohash_compare_key_int64(const void *a, const void *b);
_Bool ohash_compare_key_uint(const void *a, const void *b);
_Bool ohash_compare_key_uintmax(const void *a, const void *b);
_Bool ohash_compare_key_uint8(const void *a, const void *b);
_Bool ohash_compare_key_uint16(const void *a, const void *b);
_Bool ohash_compare_key_uint32(const void *a, const void *b);
_Bool ohash_compare_key_uint64(const void *a, const void *b);
_Bool ohash_compare_key_float(const void *a, const void *b);
_Bool ohash_compare_key_double(const void *a, const void *b);
_Bool ohash_compare_key_long_double(const void *a, const void *b);


/* bundled key hashing functions */

uintmax_t ohash_key_pointer(const OHashOptions options, const void *key);
uintmax_t ohash_key_string(const OHashOptions options, const void *key);
uintmax_t ohash_key_int(const OHashOptions options, const void *key);
uintmax_t ohash_key_intmax(const OHashOptions options, const void *key);
uintmax_t ohash_key_int8(const OHashOptions options, const void *key);
uintmax_t ohash_key_int16(const OHashOptions options, const void *key);
uintmax_t ohash_key_int32(const OHashOptions options, const void *key);
uintmax_t ohash_key_int64(const OHashOptions options, const void *key);
uintmax_t ohash_key_uint(const OHashOptions options, const void *key);
uintmax_t ohash_key_uintmax(const OHashOptions options, const void *key);
uintmax_t ohash_key_uint8(const OHashOptions options, const void *key);
uintmax_t ohash_key_uint16(const OHashOptions options, const void *key);
uintmax_t ohash_key_uint32(const OHashOptions options, const void *key);
uintmax_t ohash_key_uint64(const OHashOptions options, const void *key);
uintmax_t ohash_key_float(const OHashOptions options, const void *key);
uintmax_t ohash_key_double(const OHashOptions options, const void *key);
uintmax_t ohash_key_long_double(const OHashOptions options, const void *key);

#endif
