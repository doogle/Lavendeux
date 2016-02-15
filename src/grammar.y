%{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <wchar.h>
	#include <math.h>
	#include <fenv.h>

	#include "decorators.h"
	#include "constructs.h"
	#include "parse.h"
	#include "list.h"
	#include "language.h"

	typedef union YYSTYPE YYSTYPE;
	#include "lex.h"

	#define YYERROR_MSG(c,s) yyerror_msg(scanner, result, c, s); YYABORT;
	#define _YYERROR_MSG(c,s) result->iv=c; yyerror(scanner, result, s);
	#define YYERROR_CODE(c) yyerror_code(scanner, result, c); YYABORT;

	int yyerror (yyscan_t, wchar_t[], value*, const char*);
	int_value_t ifactorial(int_value_t in);
	void yyerror_code(yyscan_t scanner, value* result, int err);
	void yyerror_msg(yyscan_t scanner, value* result, int err, int lang_str);

%}

%define api.pure

%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {wchar_t stored_function[]}
%parse-param {value *result}

%union {
	value val;
	list lst;
}

/* Valid tokens */
%token <val> IDENTIFIER HEX BIN OCT SCI FLOAT INT
%token <val> DECORATOR EQUAL LPAREN RPAREN
%token <val> ERROR
%token <list> COMMA
%token END 0 "end of expression"
%left <val> OR
%left <val> XOR
%left <val> AND
%left <val> LSHIFT RSHIFT
%left <val> PLUS MINUS
%left <val> MUL DIV MOD
%left <val> POW
%left <val> FACTORIAL
%left <val> NOT

%type<val> expression atomic_value constant_expression assignment_expression
%type<lst> expression_list identifier_list

%%

expression:
	constant_expression {
		char type;
		int err;
		if (( err = value_type(&$1, &type) ) != NO_FAILURE) {
			YYERROR_CODE(err);
		}

		$$ = $1;
		switch (type) {
			case VALUE_FLOAT:
				decorator_float(&$1, $$.sv);
				break;
			case VALUE_INT:
				decorator_int(&$1, $$.sv);
				break;
		}

		*result = $$;
	}
	| assignment_expression {
		int_value_t iv;
		float_value_t fv;
		
		char type;
		int err;
		if (( err = value_type(&$1, &type) ) != NO_FAILURE) {
			YYERROR_CODE(err);
		}

		$$ = $1;
		switch (type) {
			case VALUE_FLOAT:
				float_value(&$1, &fv);
				swprintf($$.sv, L"%Lf", fv);
				break;
			case VALUE_INT:
				int_value(&$1, &iv);
				swprintf($$.sv, L"%lld", iv);
				break;
		}

		*result = $$;
	}
	| expression DECORATOR IDENTIFIER {
		if (!decorate($3.sv, &$1, $$.sv)) {
			YYERROR_MSG(FAILURE_INVALID_NAME, LANG_STR_INVALID_DECORATOR);
		}
		*result = $$;
	}
	;

atomic_value:
	IDENTIFIER {
		$$ = $1;
	}
	| HEX {
		$$ = $1;
	}
	| BIN {
		$$ = $1;
	}
	| OCT {
		$$ = $1;
	}
	| SCI {
		$$ = $1;
	}
	| FLOAT {
		$$ = $1;
	}
	| INT {
		$$ = $1;
	}
	| ERROR {
		YYERROR_MSG(FAILURE_INVALID_ARGS, $1.iv);
	}
	;

constant_expression:
	atomic_value {
		$$ = $1;
	}
	| LPAREN constant_expression RPAREN {
		$$ = $2;
	}
	| constant_expression OR constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value_t right_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					YYERROR_MSG(FAILURE_TYPE, LANG_STR_BOOLEAN_FLOAT);
				break;

				case VALUE_INT:
					int_value(&$1, &left_op);
					int_value(&$3, &right_op);
					
					
					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = left_op | right_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression XOR constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value_t right_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					YYERROR_MSG(FAILURE_TYPE, LANG_STR_BOOLEAN_FLOAT);
				break;

				case VALUE_INT:
					int_value(&$1, &left_op);
					int_value(&$3, &right_op);
					
					
					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = left_op ^ right_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression AND constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value_t right_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					YYERROR_MSG(FAILURE_TYPE, LANG_STR_BOOLEAN_FLOAT);
				break;

				case VALUE_INT:
					int_value(&$1, &left_op);
					int_value(&$3, &right_op);
					
					
					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = left_op & right_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression LSHIFT constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value_t right_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					YYERROR_MSG(FAILURE_TYPE, LANG_STR_BOOLEAN_FLOAT);
				break;

				case VALUE_INT:
					int_value(&$1, &left_op);
					int_value(&$3, &right_op);
					
					
					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = left_op << right_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression RSHIFT constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value_t right_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					YYERROR_MSG(FAILURE_TYPE, LANG_STR_BOOLEAN_FLOAT);
				break;

				case VALUE_INT:
					int_value(&$1, &left_op);
					int_value(&$3, &right_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = left_op >> right_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression PLUS constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			float_value_t fleft_op;
			float_value_t fright_op;
			int_value_t ileft_op;
			int_value_t iright_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					float_value(&$1, &fleft_op);
					float_value(&$3, &fright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.fv = fleft_op + fright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
				break;

				case VALUE_INT:
					int_value(&$1, &ileft_op);
					int_value(&$3, &iright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = ileft_op + iright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression MINUS constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			float_value_t fleft_op;
			float_value_t fright_op;
			int_value_t ileft_op;
			int_value_t iright_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					float_value(&$1, &fleft_op);
					float_value(&$3, &fright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.fv = fleft_op - fright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
				break;

				case VALUE_INT:
					int_value(&$1, &ileft_op);
					int_value(&$3, &iright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = ileft_op - iright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression MUL constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			float_value_t fleft_op;
			float_value_t fright_op;
			int_value_t ileft_op;
			int_value_t iright_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					float_value(&$1, &fleft_op);
					float_value(&$3, &fright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.fv = fleft_op * fright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
				break;

				case VALUE_INT:
					int_value(&$1, &ileft_op);
					int_value(&$3, &iright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = ileft_op * iright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression DIV constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			float_value_t fleft_op;
			float_value_t fright_op;
			int_value_t ileft_op;
			int_value_t iright_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					float_value(&$1, &fleft_op);
					float_value(&$3, &fright_op);

					if (fright_op == 0.0) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_DIV_BY_ZERO);
					}

					feclearexcept (FE_ALL_EXCEPT);
					$$.fv = fleft_op / fright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
				break;

				case VALUE_INT:
					int_value(&$1, &ileft_op);
					int_value(&$3, &iright_op);

					if (iright_op == 0) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_DIV_BY_ZERO);
					}

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = ileft_op / iright_op;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression MOD constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value_t right_op;
			int_value(&$1, &left_op);
			int_value(&$3, &right_op);

			if (right_op == 0) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_DIV_BY_ZERO);
			}

			feclearexcept (FE_ALL_EXCEPT);
			$$.iv = left_op % right_op;

			if (fetestexcept (FE_OVERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
			} else if (fetestexcept (FE_UNDERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
			}
		}
	}
	| constant_expression POW constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, &$3);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			float_value_t fleft_op;
			float_value_t fright_op;
			int_value_t ileft_op;
			int_value_t iright_op;
			switch ($$.type) {
				case VALUE_FLOAT:
					float_value(&$1, &fleft_op);
					float_value(&$3, &fright_op);

					$$.fv = powl(fleft_op, fright_op);	

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
				break;

				case VALUE_INT:
					int_value(&$1, &ileft_op);
					int_value(&$3, &iright_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = (int_value_t) powl(ileft_op, iright_op);

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| constant_expression FACTORIAL {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(&$1, NULL);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t left_op;
			int_value(&$1, &left_op);
			if (left_op < 0) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_FACTORIAL_LT_ZERO);
			}

			feclearexcept (FE_ALL_EXCEPT);
			$$.iv = ifactorial(left_op);

			if (fetestexcept (FE_OVERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
			} else if (fetestexcept (FE_UNDERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
			}
		}
	}
	| NOT constant_expression {
		if (wcslen(stored_function) == 0) {
			$$ = verify_expression(NULL, &$2);
			if ($$.type == VALUE_ERROR) {
				YYERROR_CODE($$.iv);
			}

			int_value_t right_op;
			int i;
			int_value_t mask;
			switch ($$.type) {
				case VALUE_FLOAT:
					YYERROR_MSG(FAILURE_TYPE, LANG_STR_BOOLEAN_FLOAT);
				break;

				case VALUE_INT:
					int_value(&$2, &right_op);

					/* Determine highest order bit to build mask*/
					i = 0;
					mask = 0;
					do {
						mask += 1 << i++;
					} while (pow(2, i) <= right_op);

					feclearexcept (FE_ALL_EXCEPT);
					$$.iv = (~(right_op)) & mask;

					if (fetestexcept (FE_OVERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
					} else if (fetestexcept (FE_UNDERFLOW)) {
						YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
					}
			}
		}
	}
	| IDENTIFIER LPAREN RPAREN {
		if (wcslen(stored_function) == 0) {
			feclearexcept (FE_ALL_EXCEPT);
			solve_function($1.sv, NULL, 0, &$$);

			if (fetestexcept (FE_OVERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
			} else if (fetestexcept (FE_UNDERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
			}

		} else if (wcscmp(stored_function, $1.sv) == 0) {
			function_remove(stored_function);
			YYERROR_MSG(FAILURE_INVALID_NAME, LANG_STR_FN_CALL_SELF);
		}
	}
	| IDENTIFIER LPAREN constant_expression RPAREN {
		if (wcslen(stored_function) == 0) {
			value args[] = { $3 };

			feclearexcept (FE_ALL_EXCEPT);
			solve_function($1.sv, args, 1, &$$);

			if (fetestexcept (FE_OVERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
			} else if (fetestexcept (FE_UNDERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
			}

		} else if (wcscmp(stored_function, $1.sv) == 0) {
			function_remove(stored_function);
			YYERROR_MSG(FAILURE_INVALID_NAME, LANG_STR_FN_CALL_SELF);
		}
	}
	| IDENTIFIER LPAREN expression_list RPAREN {
		if (wcslen(stored_function) == 0) {

			feclearexcept (FE_ALL_EXCEPT);
			solve_function($1.sv, $3.elements, $3.size, &$$);

			if (fetestexcept (FE_OVERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_OVERFLOW);
			} else if (fetestexcept (FE_UNDERFLOW)) {
				YYERROR_MSG(FAILURE_INVALID_ARGS, LANG_STR_UNDERFLOW);
			}

			list_destroy(&$3);
		} else if (wcscmp(stored_function, $1.sv) == 0) {
			function_remove(stored_function);
			YYERROR_MSG(FAILURE_INVALID_NAME, LANG_STR_FN_CALL_SELF);
		}
	}
	;

expression_list:
	constant_expression COMMA constant_expression {
		if (wcslen(stored_function) == 0) {
			if (list_create(&$$, DEFAULT_LIST_CAPACITY) != NO_FAILURE) {
				YYERROR_CODE(FAILURE_ALLOCATION);
			}

			list_add(&$$, $1);
			list_add(&$$, $3);
		}
	}
	| expression_list COMMA constant_expression {
		if (wcslen(stored_function) == 0) {
			if (list_add(&$$, $3) != NO_FAILURE) {
				YYERROR_CODE(FAILURE_ALLOCATION);
			}
		}
	}
	;

assignment_expression:
	IDENTIFIER EQUAL constant_expression {
		value *v = (value*) malloc(sizeof(value));
		if (v == NULL) {
			YYERROR_CODE(FAILURE_ALLOCATION);
		}

		*v = $3;
		if (put_variable($1.sv, v) != NO_FAILURE) {
			YYERROR_CODE(FAILURE_ALLOCATION);
		}

		$$ = $3;
	}

identifier_list:
	IDENTIFIER COMMA IDENTIFIER {
		if (list_create(&$$, DEFAULT_LIST_CAPACITY) != NO_FAILURE) {
			YYERROR_CODE(FAILURE_ALLOCATION);
		}

		list_add(&$$, $1);
		list_add(&$$, $3);
	}
	| identifier_list COMMA IDENTIFIER {
		if (list_add(&$$, $3) != NO_FAILURE) {
			YYERROR_CODE(FAILURE_ALLOCATION);
		}
	}
	;

%%

int yyerror(yyscan_t scanner, value* result, const char* msg) {
	char* pos = yyget_text(scanner);
	wchar_t buffer[EXPRESSION_MAX_LEN];
	result->iv = FAILURE_SYNTAX_ERROR;

	mbstowcs(result->sv, msg, EXPRESSION_MAX_LEN-1);
	result->sv[strlen(msg)] = L'\0';
	if (strlen(pos) != 0) {
		swprintf(buffer, L" at '%C'", pos[0]);
		wcscat(result->sv, buffer);
	}

	return 1;
}

int_value_t ifactorial(int_value_t in) {
	if (in <= 0)
		return 1;

	return ifactorial(in-1) * in;
}

void yyerror_code(yyscan_t scanner, value* result, int err) {
	switch (err) {
		case FAILURE_UNKNOWN:
			yyerror_msg(scanner, result, err, LANG_STR_ERR_UNKNOWN);
		break;

		case FAILURE_INVALID_ARGS:
			yyerror_msg(scanner, result, err, LANG_STR_ERR_INVALID_ARGS);
		break;

		case FAILURE_INVALID_NAME:
			yyerror_msg(scanner, result, err, LANG_STR_ERR_INVALID_NAME);
		break;

		case FAILURE_SYNTAX_ERROR:
			yyerror_msg(scanner, result, err, LANG_STR_ERR_SYNTAX_ERROR);
		break;

		case FAILURE_ALLOCATION:
			yyerror_msg(scanner, result, err, LANG_STR_ERR_ALLOCATION);
		break;

		case FAILURE_TYPE:
			yyerror_msg(scanner, result, err, LANG_STR_ERR_TYPE);	
		break;
	}
}

void yyerror_msg(yyscan_t scanner, value* result, int err, int lang_str) {
	char* err_str = language_char_str(lang_str);

	if (err_str != NULL) {
		_YYERROR_MSG(err, err_str);
		free(err_str);
	}
}