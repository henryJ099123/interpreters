#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
	initScanner(source);
	int line = -1;
	for(;;) {
		Token token = scanToken();
		if(token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		} else {
			printf("   | ");
		} 
		// this is bizarre, but the * lets you pass the precision as an argument
		// to printf instead of hardcoding it. So, `token.length` is given as the
		// number of characters of the string from `token.start` to print.
		// this is to accommodate the lack of a null char
		printf("%2d '%.*s'\n", token.type, token.length, token.start);

		if(token.type == TOKEN_EOF) break;
	} 
} 
