/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

%pure-parser
%lex-param {Jsonparsestate* parsestate}

%token STRING
%token NUMBER
%token TRUE
%token FALSE
%token NULL

%start value
%%

value:
	  object
	| array
	| STRING
	| NUMBER
	| NULL
	| TRUE
	| FALSE
	;
object:
	'{' members '}'
	;
array:
	'[' values ']'
	;

members:
	  member
	| members ',' member
	;

values:
	  value
	| values ',' value
	;

member:
	STRING ':' value
	;

%%

