---
title: A Hash Table
summary: Look values up by name in nearly constant time. Hash a string to a bucket, chain collisions in linked lists, and manage the heap memory yourself.
minutes: 40
difficulty: hard
id: c-systems/hash-table
mode: harness
---
Suppose you keep the names and ages of a thousand villagers. Searching an array means checking up to a thousand names every time. A **hash table** finds one in about the same time whether you have ten villagers or ten million. It is how dictionaries in Python, objects in JavaScript and maps in nearly every language work inside.

## The idea

Keep a row of **buckets**, like a wall of pigeonholes. A **hash function** turns any string into a number. Take that number modulo the number of buckets and you get a pigeonhole. To store `"alice"`, hash it, pick the bucket, put it there. To find `"alice"` later, hash it again and look in the same bucket: no searching through the others.

Here is a classic hash function, called djb2. It starts with a magic number and folds in each character:

```c
unsigned long hash_str(const char *s) {
    unsigned long h = 5381;
    for (; *s != '\0'; s++) {
        h = h * 33 + (unsigned char)*s;
    }
    return h;
}

size_t bucket = hash_str("alice") % BUCKETS;
```

## Collisions

Two different names can land in the same bucket. That is called a **collision** and it is completely normal. The simplest remedy is **chaining**: each bucket holds a linked list (you built one in the last track) of everything that landed there.

```c
typedef struct Entry {
    char *key;
    int value;
    struct Entry *next;
} Entry;

typedef struct {
    Entry *buckets[BUCKETS];
} HashTable;
```

Notice `struct Entry *next` inside the definition: the typedef name does not exist yet while the struct is being described, so the struct needs its own name for the self-reference.

## put

1. Hash the key, pick the bucket.
2. Walk that bucket's chain with `strcmp`. If the key is already there, **update its value** and stop. Never store the same key twice.
3. Otherwise make a new entry. Copy the key into fresh heap memory (the caller's string might change or vanish later), set the value, and link the entry in at the front of the chain.

## get

Hash, pick the bucket, walk the chain comparing keys, and report the value if found. Because a value could legitimately be `0` or negative, `get` returns "found or not" separately and delivers the value through a pointer.

## free

Every entry owns two heap blocks: the entry itself and its copy of the key. Free both, for every entry in every bucket, and remember to grab `next` *before* you free the entry that holds it.

## Your turn

The types and `hash_str` are provided. Write init, put, get and free. The tests insert hundreds of keys, so collisions are certain.
