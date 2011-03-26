

int
value(state, node)
{
    int token = jsonlex(state);
    switch (token) {
    case '{':
	object(state,?);
	break;
    case '[':
    case JSON_STRING:
    case JSON_NUMBER:
    case JSON_NULL:
    case JSON_TRUE:
    case JSON_FALSE:
    default: return 0;
    }

	  object
	| array
	| JSON_STRING
	| JSON_NUMBER
	| JSON_NULL
	| JSON_TRUE
	| JSON_FALSE
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
	stringLit ':' value
	;

    case '{':
    case '}':
    case '{':
    case '}':
    case ':':
    case ',':
    case JSON_STRING:
    case JSON_NUMBER:
    case JSON_NULL:
    case JSON_TRUE:
    case JSON_FALSE:
    default: return 0;
