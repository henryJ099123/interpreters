#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// load capacity
// should be tuned properly and tested
#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
} 

void freeTable(Table* table) {
	FREE_ARRAY(Entry, table->entries, table->capacity);
	initTable(table);
} 

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
	uint32_t index = key->hash % capacity;
	// this is the earliest earliest_tombstone
	Entry* earliest_tombstone = NULL;

	// searches for an entry
	for(;;) {
		Entry* entry = &entries[index];
		if(entry->key == NULL) {
			// this is a truly empty bucket
			// if we have passed a tombstone (i.e. it is *not* null)
			// then we return that, for reuse e.g. for setting
			// otherwise we return the entry
			if(IS_NIL(entry->value)) {// empty entry
				return earliest_tombstone != NULL ? earliest_tombstone : entry;
			}
			// this is not a truly empty entry, i.e. a tombstone
			// we set our earliest 
			else {
				if (earliest_tombstone == NULL) earliest_tombstone = entry;
			}
		} else if(entry->key == key) {
			return entry;
		}
		index = (index + 1) % capacity;
	} 
	// no need to check if we *can't* find a bucket because this is impossible
} 

static void adjustCapacity(Table* table, int capacity) {
	Entry* entries = ALLOCATE(Entry, capacity);
	// zero out the array
	for(int i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = NIL_VAL;
	} 

	// copy over the elements
	table->count = 0;
	for(int i = 0; i < table->capacity; i++) {
		Entry* entry = &table->entries[i];
		if(entry->key == NULL) continue; // conveniently ignores tombstones
		Entry* dest = findEntry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	} 

	// free the old array
	FREE_ARRAY(Entry, table->entries, table->capacity);

	// set the new array
	table->entries = entries;
	table->capacity = capacity;
} 

bool tableSet(Table* table, ObjString* key, Value value) {
	// regrowing requires refinding places for everything
	if(table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	} 
	Entry* entry = findEntry(table->entries, table->capacity, key);
	// is this a new entry, or did we overwrite something?
	bool isNewKey = entry->key == NULL;
	if(isNewKey && IS_NIL(entry->value)) table->count++;
	entry->key = key;
	entry->value = value;
	return isNewKey;
} 

bool tableGet(Table* table, ObjString* key, Value* value) {
	if(table->count == 0) return false; // this is an optimization
	Entry* entry = findEntry(table->entries, table->capacity, key);
	if(entry->key == NULL) return false; 
	
	// value is just a return thing
	*value = entry->value;
	return true;
} 

bool tableKeyExists(Table* table, ObjString* key) {
    if(table->count == 0) return false;
    Entry* entry = findEntry(table->entries, table->capacity, key);
    return entry->key != NULL;
} 

bool tableDelete(Table* table, ObjString* key) {
	if(table->count == 0) return false;

	// get entry
	Entry* entry = findEntry(table->entries, table->capacity, key);
	if(entry->key == NULL) return false;

	// place tombstone: null key, but true value?
	entry->key = NULL;
	entry->value = BOOL_VAL(true);
	return true;
} 

ObjString* tableFindKey(Table* table, Value value) {
	for(int i = 0; i < table->capacity; i++) {
		if(table->entries[i].key == NULL) continue;
		if(valuesEqual(table->entries[i].value, value))
			return table->entries[i].key;
	} 
	return NULL;
} 

// helper function to copy over one hash table to another
void tableAddAll(Table* from, Table* to) {
	for(int i = 0; i < from->capacity; i++) {
		Entry* entry = &from->entries[i];
		if(entry->key != NULL)
			tableSet(to, entry->key, entry->value);
	} 
} 

// mostly redundant with findEntry, except we use a raw char and memcmp
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
	if(table->count == 0) return NULL;
	uint32_t index = hash % table->capacity;
	for(;;) {
		Entry* entry = &table->entries[index];
		if(entry->key == NULL) {
			if(IS_NIL(entry->value)) return NULL;
		} else if(entry->key->length == length && 
				  entry->key->hash == hash &&
				  memcmp(entry->key->chars, chars, length) == 0) {
			return entry->key;
		} 
		index = (index + 1) % table->capacity;
	} 
} 

// the string intern table just uses the key of each entry
void tableRemoveWhite(Table* table) {
    for(int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if(entry->key != NULL && !entry->key->obj.isMarked)
            tableDelete(table, entry->key);
    } 
} 

void markTable(Table* table) {
    for(int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        markObject((Obj*)entry->key);
        markValue(entry->value);
    } 
} 
