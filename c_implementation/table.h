#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

// this language only needs strings as keys
typedef struct {
	ObjString* key;
	Value value;
} Entry;

typedef struct {
	int count;
	int capacity;
	Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableKeyExists(Table* table, ObjString* key);
bool tableDelete(Table* table, ObjString* key);
ObjString* tableFindKey(Table* table, Value value);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(Table* table);
void markTable(Table* table);

#endif
