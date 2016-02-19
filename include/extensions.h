#ifndef EXTENSIONS_H
	#define EXTENSIONS_H

	#define EXTENSIONS_PATH "./extensions"
	#define EXTENSIONS_FUNCTION "call"

	#include <stdlib.h>
	#include <wchar.h>

	#include "parse.h"
	
	int extensions_init( void );
	int extensions_homeset( void );
	int extensions_available( void );
	void extensions_destroy( void );
	int extensions_call(const char*, value[], int, value*);
#endif