// a simple if not simplistic implementation of hash map
// both key type and value types are bytes

#include <stddef.h>
#include <string.h>
#include <sys/param.h>

struct string {
    char* str;
    size_t n;
};
const struct string nullString =  {
    .n = 0,
    .str = NULL,
};

int compareString(struct string str1, struct string str2) {
    int m = MIN(str1.n, str2.n);
    int result = memcmp(str1.str, str2.str, m);
    if (result != 0) {
        return result;
    }
    if (str1.n == str2.n) return 0;
    if (str1.n > str2.n) return 1;
    if (str1.n < str2.n) return -1;
}

struct listElem;

struct list {
    struct listElem *head, *tail;
};

// a list of strings
struct listElem {
    struct list* prev;
    struct string key;
    struct string value;
    struct list* next;
};

struct listElem* newListElem(struct string key, struct string value) {
    struct listElem *elem = malloc(sizeof(struct listElem));
    elem->key = key;
    elem->value = value;
    elem->prev = NULL;
    elem->next = NULL;
    return elem;
}

typedef size_t (*hashFunc)(struct string* str);

struct hashmap {
    int max_bucket;
    struct list buckets;
    hashFunc hash;
};

struct hashmap* newHashmap(int buckets, hashFunc hash) {
    struct hashmap* hash_map = malloc(sizeof(struct hashmap));
    hash_map->max_bucket = buckets;
    hash_map->hash = hash;
    hash_map->buckets.head = NULL;
    hash_map->buckets.tail = NULL;
}

void append(struct list* l, struct listElem* elem) {
    if (l->tail == NULL) {
        assert(l->head == NULL);
        l->head =elem, l->tail = elem;
    } else {
        l->tail->next = elem;
        elem->prev = l->tail;
        elem->next = NULL;
    }
}

struct listElem* search(struct list* l, struct string key) {
    struct listElem *elem;
    for (elem = l->head; elem; elem = elem->next) {
        if (compareString(elem->key, key) == 0) {
            return elem;
        }
    }
    return NULL;
}

void remove(struct list* l, struct listElem* elem) {
    struct listElem* e;
    for (e = l->head; e; e = e->next) {
        if (e == elem) {
            struct listElem *prev = e->prev, *next = e->next;
            if (prev) prev->next = next;
            if (next) next->prev = prev;
            return;
        }
    }
}