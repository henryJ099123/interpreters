#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
	char line[1024];
	for(;;) {
		printf("> ");

		if(!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		} 

		interpret(line);
	} 
} 

static char* readFile(const char* path) {
	FILE* file = fopen(path, "rb");
	//file fails to open:
	if(file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	} 

	// get size of file
	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	// read those bytes, append null char
	char* buffer = (char*) malloc(fileSize+1);
	// file fails to allocate memory for string:
	if(buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	} 
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	// file fails to read everything:
	if(bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	} 
	buffer[bytesRead] = '\0';

	// return the string
	fclose(file);
	return buffer;
} 

static void runFile(const char* path) {
	char* source = readFile(path);
	InterpretResult result = interpret(source);
	free(source);
	if(result == INTERPRET_COMPILE_ERROR) exit(65);
	if(result == INTERPRET_RUNTIME_ERROR) exit(70);
	
} 

int main(int argc, const char* argv[]) {
	initVM();
	if(argc == 1)
		repl();
	else if(argc == 2)
		runFile(argv[1]);
	else {
		fprintf(stderr, "Usage: clox [path]\n");
		exit(64);
	}

	freeVM();
	return 0;
} 
