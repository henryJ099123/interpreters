# Ch. 20: Hash Tables

- Looking up variables required to add variables (and also fields on instances)
- However C has no built-in hash table
- **keys** and **values**, each of which is an **entry** in the table
- their benefit: hash table returns a value from a key in constant time *regardless of the table's size* in the average case

## An array of buckets

- I've already done this...but I'll hear you out, Nystrom
- Each "spot" for a key is called a **bucket**
- for a fixed number of possible keys, the table can be static and it just becomes a glorified array

### Load factor and wrapped keys

- What if we allow variables up to 8 characters long?
- pack all 8 chars into a 64-bit integer
    - but can't just have this as an address, otherwise we need 295 petabytes of data
- goal: allocate array with more than enough capacity for the needed entries, but not *unreasonably* large
    - map the keys to that smaller range via the modulus function
> progressive size of a hash table can be powers of two, but some implementations prefer prime numbers
- This works except when keys **collide** on a single bucket
    - this can be mitigated by changing the array's size
- **load factor**: the number of things in the table compared to its capacity
    - resize array based on load factor

## Collision resolution

- chances of collisions increases dramatically with the number of entries in the hash table
    - birthday paradox, anyone?

### Separate chaining

- have each bucket contain a linked list of entries that fall into that bucket
- walk the list to find a match
    - worst case: $\mathcal{O}(n)$
    - easy to avoid worst case via the load factor
- not great for CPU caches and pointer overhead

### Open addressing

- also known as **closed hashing**, all entries are in the bucket array
    - instead of chaining, just find a new bucket by probing
    - **probe sequence**: the order of examining buckets to find the key
- many different probing algorithms: cuckoo hashing, Robin Hood hashing, double hashing
- this book does **linear probing**
- good for cache, bad for **clustering**: lots of entries with numerically similar key values leads to many collisions
- It's okay tho because the cache friendliness is definitely worth it
- the interleaving is not that deep because it's still walking the list
    - however, we also now have the case that you traverse over entries of a different original bucket

## Hash functions

- we need to take an any-length string and convert it to a fixed-size integer
- this is the job of a **hash function**:
    - deterministic
    - uniform (it's onto and the distribution is uniform among the codomain)
    - fast
- there are *many* hash functions, and some people study these for real

## Building a hash table

- it's a simple data structure (I know)
- strings need to be hashed first, so we need to implement the hash function
    - Nystrom chose FNV-1a
- the hash function should look at all the bits of the key string
    - however, walking an entire string to calculate a hash can be slow
    - thus, we should cache the strings (store in the ObjString)
- immutable strings in Lox means the hash can be calculated up-front
    - store in the ObjString
- oh this is *really* annoying after doing the exercise 1 in the previous chapter
- the code uses `==` for comparing ObjStrings?

### Deleting entries

- as important to remove as it is to add
    - easy to delete in chaining, as you just remove a node from a linked list
- deletion actualy not that common...mostly in a small edge case
    - tricky with open-addressing (because of the interleaving!)
- if we delete an entry, an implicit probing sequence may be interrupted
    - fixed by adding **tombstones** that replace a deletion with a sentinel entry
    - treated as an entry when probing, but as empty when setting
- problem: we could end up with an array of just tombstones with no empty buckets
    - should tombstones be counted in the load factor?

### Counting tombstones

- treating tombstones like full buckets may result in a bigger array than needed, as it artificially inflates the load factor
    - but treating tombstones like empty could lead to no actual empty buckets
    - so we treat them as full buckets
    - thus, we don't reduce the count when deleting an entry, and don't increment if filling a tombstone
- we also can't copy over tombstones when copying a hash table
- a small issue: deletions tend to be rare, so the delete function should be "longer" to shorten lookups
    - however, tombstones makes deletions fast and penalizes lookups
    - but in Nystrom's testing, the tombstones were faster overall
- tombstones doesn't push the work of deletion to other operations but makes it *lazy*

## String interning

- the hash table uses `==` to compare two strings for equality which just doesn't work if the strings are in different places in memory
    - we could do character-by-character matching, but this is slow
    - we could compare hashes and *then* do char-by-char matching, but this is also slower
- we do **string interning**: two strings that have the same characters but are in different places in memory are basically duplicates
    - create a collection of "interned" strings, i.e. all literals
    - any string in this collection is textually distinct from the others
    - i.e. a storage of all string literals, allows comparison via pointers
- so, value equality is trivially simple, as long as the VM can intern them all
- this means string comparison is *very fast*
    - very good for method lookup
- OK so, big problem: ***this all breaks if you do the ch19 exercise number 1***
    - you can't return strings easily if they're literally attached to the value
        - and the value *has to change* easily
        - I have to run back basically all the old code...
    - wait...I might be wrong...because the table is of ObjStrings, not of regular ones...

## Exercises

1. In clox, only need string keys, so the table built is hardcoded for that key type.
If we were to expose the hash table to Lox users as a first-class collection,
support different kinds of keys would be nice.
Add support for keys of the other primitive types.
Later, clox will support user-defined classes.
If we want to support keys that are *instances of those classes*, what complexity does that add?
    - well, to answer the last thing: hashing complexity (how to hash)
    - but then also, problems arise with *mutability*
    - the nice thing about these first couple is that they are not mutable
2. Hash tables have a lot of knobs to tweak.
    - look at a few hash table implementations to see why they did things this way.
3. Benchmarking is really really hard. Write some benchmark programs.

