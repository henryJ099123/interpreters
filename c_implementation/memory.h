#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) \
	(type*) reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// this is a macro function. use \ to do newlines in one.
// I'm fairly certain macros treat things as if you transplanted them there,
// so you need parentheses around literally everything.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// calls `reallocate`, I believe Nystrom's master function
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
	(type*)reallocate(pointer, sizeof(type) * (oldCount), \
		sizeof(type) * (newCount))

//no need for `(type*)` as this will just return a null ptr
#define FREE_ARRAY(type, pointer, count) \
	reallocate(pointer, sizeof(type) * (count), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void markObject(Obj* object);
void markValue(Value value);
void collectGarbage();
void freeObjects();

#endif
