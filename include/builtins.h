#ifndef BUILTINS_H
	#define BUILTINS_H

	#include "parse.h"
	#include "hashing.h"
	#include <math.h>

	/* A builtin function looks like this */
	typedef int(*builtin)(value[], value*, int);
	typedef struct {
		builtin fn;
		int n_args;
	} builtin_function;

	/* Store builtin functions */
	hash_table builtins;

	int builtins_init( void );
	void builtin_destroy(void*);
	void builtins_destroy( void );
	int builtin_put(const wchar_t*, builtin, int);
	int is_builtin(const wchar_t*);
	int call_builtin(const wchar_t*, value[], int, value*, int);

	/* Builtins */
	int builtin_floor(value[], value*, int);
	int builtin_ceil(value[], value*, int);
	int builtin_round(value[], value*, int);
	int builtin_abs(value[], value*, int);

	int builtin_tan(value[], value*, int);
	int builtin_cos(value[], value*, int);
	int builtin_sin(value[], value*, int);

	int builtin_atan(value[], value*, int);
	int builtin_acos(value[], value*, int);
	int builtin_asin(value[], value*, int);

	int builtin_tanh(value[], value*, int);
	int builtin_cosh(value[], value*, int);
	int builtin_sinh(value[], value*, int);

	int builtin_log10(value[], value*, int);
	int builtin_ln(value[], value*, int);
	int builtin_log(value[], value*, int);

	int builtin_sqrt(value[], value*, int);
	int builtin_root(value[], value*, int);

	int builtin_strlen(value[], value*, int);
	int builtin_substr(value[], value*, int);
	int builtin_chr(value[], value*, int);
	int builtin_ord(value[], value*, int);

	/* For static table entries */
	typedef struct { const wchar_t* name; builtin fn; unsigned int args; } static_builtin_fn;

	/* Builtin function definitions */
	static const static_builtin_fn builtin_function_declarations[] = {
		{ L"ceil", builtin_ceil, 1 },
		{ L"floor", builtin_floor, 1 },
		{ L"round", builtin_round, 2 },
		{ L"abs", builtin_abs, 1 },

		{ L"tan", builtin_tan, 1 },
		{ L"cos", builtin_cos, 1 },
		{ L"sin", builtin_sin, 1 },

		{ L"atan", builtin_atan, 1 },
		{ L"acos", builtin_acos, 1 },
		{ L"asin", builtin_asin, 1 },
		
		{ L"tanh", builtin_tanh, 1 },
		{ L"cosh", builtin_cosh, 1 },
		{ L"sinh", builtin_sinh, 1 },

		{ L"log10", builtin_log10, 1 },
		{ L"ln", builtin_ln, 1 },
		{ L"log", builtin_log, 2 },

		{ L"sqrt", builtin_sqrt, 1 },
		{ L"root", builtin_root, 2 },

		{ L"strlen", builtin_strlen, 1 },
		{ L"substr", builtin_substr, 3 },
		{ L"chr", builtin_chr, 1 },
		{ L"ord", builtin_ord, 1 },
	};
#endif