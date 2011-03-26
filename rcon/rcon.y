/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

%pure-parser
%lex-param {Jsonparsestate* parsestate}

%token STRING
%token WORD
%token NUMBER
%token TRUE
%token FALSE
%token NULL

%start rcon
%%

rcon:
	  members
	| map
	;

value:
	  map
	| array
	| STRING
	| WORD
	| NUMBER
	| NULL
	| TRUE
	| FALSE
	;
map:
	'{' members '}'
	;
array:
	'[' values ']'
	;

members:
	  pair
	| members pair
	;

values:
	  value
	| values value
	;

pair:
	  STRING ':' value
	| WORD ':' value
	;

%%

