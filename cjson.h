#ifndef GMTEIXEIRA_JSON_H
#define GMTEIXEIRA_JSON_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define IS_NUMERIC(c) ((c > 0x2f && c < 0x3a) || c == 0x2d)
#define IS_WHITESPACE(c) (c == 0x0d || c == 0x0a || c == 0x09 || c == 0x20)
#define JSON_OBJECT_SIZE (4)
#define UTF8_ZERO (0x30)

#define GET_CHAR(state) (state->input[state->curIndex])
#define SWITCH_WHITESPACE_CASES case '\n': \
	case '\t':			   \
	case '\r':			   \
	case ' '

#define SWITCH_NUMERIC_CASES case '0': \
	case '1':		      \
	case '2':		      \
	case '3':		      \
	case '4':		      \
	case '5':		      \
	case '6':		      \
	case '7':		      \
	case '8':		      \
	case '9':		      \
	case '-'

#define SWITCH_NUMBER_CASES case '0': \
	case '1':		      \
	case '2':		      \
	case '3':		      \
	case '4':		      \
	case '5':		      \
	case '6':		      \
	case '7':		      \
	case '8':		      \
	case '9'

enum JsonType {
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_DOUBLE,
	JSON_STRING,
	JSON_TRUE,
	JSON_FALSE,
	JSON_NULL
};

struct JsonNode {
	enum JsonType type;
	char data[]; 
};

/*
  [4] numNodes
  [4] offset to offsets
  [? + ?] id + data
  [4 * ?] offsets
 */
struct JsonObject {
	size_t numNodes;
	char data[];
};

/*
  [4] numNodes
  [4] offset to offsets
  [?] data
  [4 * numNodes] offsets

 */
struct JsonArray {
	size_t numNodes;
	char data[];
};

struct ParseState {
	char **buffer;
	const char *const input;
	size_t curIndex;
	const size_t bufferSize;
};

enum JsonParseStatus {
	JSON_PARSE_OK = 0,
	JSON_PARSE_UNEXPECTED_END = -1,
	JSON_PARSE_ILLEGAL_CHAR = -2
};

// Parsing functions:
enum JsonParseStatus parseJson(const char *const input, struct JsonNode **node);
enum JsonParseStatus parseRoot(struct ParseState *state);
enum JsonParseStatus parseId(struct ParseState *state);
enum JsonParseStatus parseObject(struct ParseState *state);
enum JsonParseStatus parseString(struct ParseState *state);
enum JsonParseStatus parseArray(struct ParseState *state);
enum JsonParseStatus parseNode(struct ParseState *state);
enum JsonParseStatus parseNumeric(struct ParseState *state);
enum JsonParseStatus parseExponent(struct ParseState *state, double num);
enum JsonParseStatus parseTrue(struct ParseState *state);
enum JsonParseStatus parseFalse(struct ParseState *state);
enum JsonParseStatus parseNull(struct ParseState *state);

// Utilitary functions:
size_t calculateBufferSize(const char *const input);
double stringToDouble(const char *const input, unsigned int start, unsigned int end);
double powerOf(double num, int exp);

enum JsonParseStatus parseJson(const char *const input, struct JsonNode **node)
{
	size_t bufferSize = calculateBufferSize(input);
	void *data = malloc(sizeof(size_t) * (bufferSize) * 2);

	void *another = data;

	struct ParseState parseState = {
		.buffer = (char**)&data,
		.input = input,
		.curIndex = 0,
		.bufferSize = bufferSize
	};
	
	enum JsonParseStatus status = parseRoot(&parseState);
	
	*node = another;
	return status;

}

size_t calculateBufferSize(const char *const input)
{
	size_t size = 0;
	unsigned int index = 0;
	char curChar;
	while ((curChar = input[index]) != '\0') {
		if (!IS_WHITESPACE(curChar)) {
			size++;
		}

		index++;
	}

	return size;
}


enum JsonParseStatus parseRoot(struct ParseState *parseState)
{
	char curChar;

	while (1) {
		curChar = GET_CHAR(parseState);

		switch (curChar) {
		case '{':
			return parseObject(parseState);
		case '[':
			return parseArray(parseState);
		SWITCH_WHITESPACE_CASES:
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		default:
			return JSON_PARSE_ILLEGAL_CHAR;
		}

		parseState->curIndex++;
	}
}

enum JsonParseStatus parseId(struct ParseState *state)
{
	int isEscaped = 0;
	
	while (1) {
		char curChar = GET_CHAR(state);
		
		switch (curChar) {
		case '"':
			state->curIndex++;
			unsigned int startIndex = state->curIndex;
			while (1) {
				curChar = GET_CHAR(state);
				switch (curChar) {
				case '"':
					if (isEscaped) {
						isEscaped = !isEscaped;
					} else {
						size_t stringSize = state->curIndex - startIndex;
						if (stringSize > 0) {
							memcpy(*state->buffer, state->input + startIndex, stringSize);
							*state->buffer += stringSize;
						}

						*(char*)*state->buffer = '\0';
						*state->buffer += 1;
						state->curIndex++;

						return JSON_PARSE_OK;
					}
					break;
				case '\\':
					isEscaped = isEscaped ? 0 : 1;
					break;
				case '\0':
					return JSON_PARSE_UNEXPECTED_END;
				}

				state->curIndex++;
			}

			break;
		SWITCH_WHITESPACE_CASES:
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		default:
			return JSON_PARSE_ILLEGAL_CHAR;
		}
		
		state->curIndex++;
	}
}

enum JsonParseStatus parseObject(struct ParseState *state)
{
	*(enum JsonType*)*state->buffer = JSON_OBJECT;
	*state->buffer += sizeof(enum JsonType);
	
	char *nodeStart = *state->buffer;

	size_t *offsetToOffsets = (size_t*)*state->buffer;
	*state->buffer += sizeof(size_t);
	
	size_t *offsets = (size_t*)(*state->buffer + state->bufferSize);

	state->curIndex++;
	unsigned int numParsedNodes = 0;
	while (1) {

		char curChar = GET_CHAR(state);
		switch (curChar) {
		case '}':
			memcpy(*state->buffer, offsets - numParsedNodes, sizeof(size_t) * numParsedNodes);
			
			*offsetToOffsets = *state->buffer - nodeStart;
			*state->buffer += sizeof(size_t) * numParsedNodes;
			state->curIndex++;
			return JSON_PARSE_OK;
		case ':':
			state->curIndex++;
			
			enum JsonParseStatus status;
			if ((status = parseNode(state)) < 0) {
				return status;
			}
			break;
		SWITCH_WHITESPACE_CASES:
			state->curIndex++;
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		case ',':
			state->curIndex++;
		default:
			*(offsets - numParsedNodes) = *state->buffer - nodeStart;
			numParsedNodes++;
			
			if ((status = parseId(state)) < 0) {
				return status;
			}
		}
	}
}

enum JsonParseStatus parseString(struct ParseState *state)
{
	*(enum JsonParseStatus*)*state->buffer = JSON_STRING;
	*state->buffer += sizeof(enum JsonParseStatus);


	int isEscaped = 0;
	state->curIndex++;
	unsigned int startIndex = state->curIndex;
	while (1) {

		char curChar = GET_CHAR(state);
		
		switch (curChar) {
		case '"':
			if (isEscaped) {
				isEscaped = !isEscaped;
			} else {
				size_t stringSize = state->curIndex - startIndex;
				if (stringSize > 0) {
					memcpy(*state->buffer, state->input + startIndex, stringSize);
					*state->buffer += stringSize;
				}

				*(char*)*state->buffer = '\0';
				*state->buffer += 1;
				state->curIndex++;

				return JSON_PARSE_OK;
			}
			break;
		case '\\':
			isEscaped = isEscaped ? 0 : 1;
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		}
		
		state->curIndex++;
	}
}

enum JsonParseStatus parseArray(struct ParseState *state)
{
	/* sets the node type */
	*(enum JsonType*)*state->buffer = JSON_ARRAY;
	*state->buffer += sizeof(enum JsonType);

	char *nodeStart = *state->buffer;

	size_t *offsetToOffsets = (size_t*)*state->buffer;
	*state->buffer += sizeof(size_t);
	
	size_t *offsets = (size_t*)(*state->buffer + state->bufferSize);
	unsigned int numParsedNodes = 0;

	state->curIndex++;
	while (1) {

		char curChar = GET_CHAR(state);
		switch (curChar) {
		case ']':
			memcpy(*state->buffer, offsets - numParsedNodes, sizeof(size_t) * numParsedNodes);
			*offsetToOffsets = *state->buffer - nodeStart;
			*state->buffer += sizeof(size_t) * numParsedNodes;
			state->curIndex++;
			return JSON_PARSE_OK;
		SWITCH_WHITESPACE_CASES:
			state->curIndex++;
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		case ',':
			state->curIndex++;
		default:
			*(offsets - numParsedNodes) = *state->buffer - nodeStart;

			enum JsonParseStatus status;
			if ((status = parseNode(state)) < 0) {
				return status;
			}

			numParsedNodes++;
		}
	}
}

enum JsonParseStatus parseNode(struct ParseState *state)
{
	while (1) {
		
		char curChar = GET_CHAR(state);
		switch (curChar) {
		case '{':
			return parseObject(state);
		case '[':
			return parseArray(state);
		SWITCH_NUMERIC_CASES:
			return parseNumeric(state);
		case '"':
			return parseString(state);
		case 't':
			return parseTrue(state);
		case 'f':
			return parseFalse(state);
		case 'n':
			return parseNull(state);
		SWITCH_WHITESPACE_CASES:
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		default:
			return JSON_PARSE_ILLEGAL_CHAR;
		}

		state->curIndex++;
	}
}

double stringToDouble(const char *const input, unsigned int start, unsigned int end)
{
	int sign = 1;
	if (input[start] == '-') {
		sign = -1;
	} else {
		--start;
	}

	double num = 0;
	switch (end - start) {
	case 15: num += ((double)(input[++start]) - UTF8_ZERO) * 1000000000000000;
	case 14: num += ((double)(input[++start]) - UTF8_ZERO) * 100000000000000;
	case 13: num += ((double)(input[++start]) - UTF8_ZERO) * 10000000000000;
	case 12: num += ((double)(input[++start]) - UTF8_ZERO) * 1000000000000;
	case 11: num += ((double)(input[++start]) - UTF8_ZERO) * 100000000000;
	case 10: num += ((double)(input[++start]) - UTF8_ZERO) * 10000000000;
	case 9: num += ((double)(input[++start]) - UTF8_ZERO) * 1000000000;
	case 8: num += ((double)(input[++start]) - UTF8_ZERO) * 100000000;
	case 7: num += ((double)(input[++start]) - UTF8_ZERO) * 10000000;
	case 6: num += ((double)(input[++start]) - UTF8_ZERO) * 1000000;
	case 5: num += ((double)(input[++start]) - UTF8_ZERO) * 100000;
	case 4: num += ((double)(input[++start]) - UTF8_ZERO) * 10000;
	case 3: num += ((double)(input[++start]) - UTF8_ZERO) * 1000;
	case 2: num += ((double)(input[++start]) - UTF8_ZERO) * 100;
	case 1: num += ((double)(input[++start]) - UTF8_ZERO) * 10;
	case 0: num += ((double)(input[start]) - UTF8_ZERO);
	}

	return num * sign;
}

double powerOf(double num, int exp)
{
	double result = 1;
	double absolute = exp < 0 ? -exp : exp;
	if (exp >= 0) {
		for (int i = absolute - 1; i != 0; --i) {
			result *= num;
		}
	} else {
		for (int i = absolute - 1; i != 0; --i) {
			result /= num;
		}
	}


	return result;
}

enum JsonParseStatus parseNumeric(struct ParseState *state)
{
	unsigned int startIndex = state->curIndex;
 
	while (1) {
		
		char curChar = GET_CHAR(state);        
		switch (curChar) {
			
		SWITCH_NUMERIC_CASES:
			break;
		case '.':;
			double integerPart = stringToDouble(state->input, startIndex, state->curIndex - 1);
			state->curIndex++;
			startIndex = state->curIndex;
			while (1) {
				curChar = GET_CHAR(state);
				switch (curChar) {
				case '\0':
					return JSON_PARSE_UNEXPECTED_END;
				SWITCH_NUMBER_CASES:
					break;
				case 'e':
				case 'E':;
					double partialFraction = stringToDouble(state->input, startIndex, state->curIndex - 1);
					double partialResult = integerPart + (partialFraction / powerOf(10, state->curIndex - startIndex));
					state->curIndex++;
					return parseExponent(state, partialResult);
				default:;
					double fractionPart = stringToDouble(state->input, startIndex, state->curIndex - 1);
					double result = integerPart + (fractionPart / powerOf(10, state->curIndex - startIndex));
					*(enum JsonType*)*state->buffer = JSON_DOUBLE;
					*state->buffer += sizeof(enum JsonType);
					memcpy(*state->buffer, &result, sizeof(double));
					*state->buffer += sizeof(double);
					return JSON_PARSE_OK;
				}
				
				state->curIndex++;
			}
                case 'e':
		case 'E':;
			double partialResult = stringToDouble(state->input, startIndex, state->curIndex - 1);
			state->curIndex++;
			return parseExponent(state, partialResult);
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		default:;
			double result = stringToDouble(state->input, startIndex, state->curIndex - 1);
			*(enum JsonType*)*state->buffer = JSON_DOUBLE;
			*state->buffer += sizeof(enum JsonType);
			memcpy(*state->buffer, &result, sizeof(double));
			*state->buffer += sizeof(double);
			return JSON_PARSE_OK;
		}
        
		state->curIndex++;
	}
}

enum JsonParseStatus parseExponent(struct ParseState *state, double num)
{

	unsigned int startIndex = state->curIndex;
	while (1) {
		
		char curChar = GET_CHAR(state);
		switch (curChar) {
		SWITCH_NUMERIC_CASES:
		case '+':
			break;
		case '\0':
			return JSON_PARSE_UNEXPECTED_END;
		default:;
			double exp = stringToDouble(state->input, startIndex, state->curIndex - 1);
			*(enum JsonType*)*state->buffer = JSON_DOUBLE;
			*state->buffer += sizeof(enum JsonType);
			exp = powerOf(10, exp);
			num *= exp;
			memcpy(*state->buffer, &num, sizeof(double));
			*state->buffer += sizeof(double);

			return JSON_PARSE_OK;
		}
		
		state->curIndex++;
	}
}

enum JsonParseStatus parseTrue(struct ParseState *state)
{
	char trueStr[3];
	trueStr[0] = state->input[++state->curIndex];
	trueStr[1] = state->input[++state->curIndex];
	trueStr[2] = state->input[++state->curIndex];
	state->curIndex++;
	
	if (trueStr[0] == 'r' &&
	    trueStr[1] == 'u' &&
	    trueStr[2] == 'e') {
		*(enum JsonType*)*state->buffer = JSON_TRUE;
		*state->buffer += sizeof(enum JsonType);

		*(char*)*state->buffer = 1;
		*state->buffer += sizeof(char);

		return JSON_PARSE_OK;
	} else {
		return JSON_PARSE_ILLEGAL_CHAR;
	}
	
	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus parseFalse(struct ParseState *state)
{
	char falseStr[4];
	falseStr[0] = state->input[++state->curIndex];
	falseStr[1] = state->input[++state->curIndex];
	falseStr[2] = state->input[++state->curIndex];
	falseStr[3] = state->input[++state->curIndex];
	state->curIndex++;
	
	if (falseStr[0] == 'a' &&
	    falseStr[1] == 'l' &&
	    falseStr[2] == 's' &&
	    falseStr[3] == 'e') {
		*(enum JsonType*)*state->buffer = JSON_FALSE;
		*state->buffer += sizeof(enum JsonType);

		*(char*)*state->buffer = 0;
		*state->buffer += sizeof(char);

		
		return JSON_PARSE_OK;
	} else {
		return JSON_PARSE_ILLEGAL_CHAR;
	}
	
	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus parseNull(struct ParseState *state)
{
	char nullStr[3];
	nullStr[0] = state->input[++state->curIndex];
	nullStr[1] = state->input[++state->curIndex];
	nullStr[2] = state->input[++state->curIndex];
	state->curIndex++;
	
	if (nullStr[0] == 'u' &&
	    nullStr[1] == 'l' &&
	    nullStr[2] == 'l') {
		*(enum JsonType*)*state->buffer = JSON_TRUE;
		*state->buffer += sizeof(enum JsonType);

		*(char*)*state->buffer = 0;
		*state->buffer += sizeof(char);
		
		return JSON_PARSE_OK;
	} else {
		return JSON_PARSE_ILLEGAL_CHAR;
	}
	
	return JSON_PARSE_UNEXPECTED_END;
}

/* printing */
#include <stdio.h>
void printJson(struct JsonNode *node, int level);
void printObject(struct JsonObject *object, int level);
void printArray(struct JsonArray *array, int level);
void printString(char *node, int level);
void printDouble(double num, int level);

void printLevel(int level) {
	for (int i = 0; i < level; i++) {
		printf(" ");
	}
} 

void printTrue(int level) {
	printLevel(level);
	printf("true");
}

void printFalse(int level) {
	printLevel(level);
	printf("false");
}

void printNull(int level) {
	printLevel(level);
	printf("null");
}

void printJson(struct JsonNode *node, int level)
{
	switch (node->type) {
	case JSON_OBJECT:
		printObject((struct JsonObject*)node->data, level);
		break;
	case JSON_ARRAY:
		printArray((struct JsonArray*)node->data, level);
		break;
	case JSON_DOUBLE:
		printDouble(*(double*)node->data, level);
		break;
	case JSON_STRING:
		printString(node->data, level);
		break;
	case JSON_TRUE:
		printTrue(level);
		break;
	case JSON_FALSE:
		printFalse(level);
		break;
	case JSON_NULL:
		printNull(level);
		break;
	}
}

void printObject(struct JsonObject *object, int level)
{
	printLevel(level);
	printf("{\n");
	level++;
	size_t numEntries = object->numNodes;
	size_t *offsets = (size_t*)object->data;
	
	for (size_t i = 0; i < numEntries; i++) {
		char *id = (char*)object + offsets[i];
		char *data = id + ((strlen(id) + 1) * sizeof(char));

		printLevel(level);
		printf("\"%s\" :", id);
		
		printJson((struct JsonNode*)data, level);

		if (i != numEntries - 1) {
			printLevel(level);
			printf(", \n");	
		} else {
			printf("\n");
		}
	}

	level--;
	printLevel(level);
	printf("}\n");
}

void printArray(struct JsonArray *array, int level)
{

	printLevel(level);
	printf("[");
	level++;
	size_t numNodes = array->numNodes;
	size_t *offsets = (size_t*)array->data;
	
	for (size_t i = 0; i < numNodes; i++) {

		struct JsonNode *node = (struct JsonNode*)((char*)array + offsets[i]);
		printf("\n");
		printJson(node, level);

		
		if (i != numNodes - 1) {
			printf(",");	
		} else {
			printf("\n");
		}
	}

	level--;
	if (numNodes != 0) {
		printLevel(level);
	}
	printf("]\n");
	
}

void printString(char *node, int level)
{
	printLevel(level);
	printf("\"%s\"", (char*)node);
}

void printDouble(double num, int level)
{
	printLevel(level);
	printf("%f", num);
}

#endif /* GMTEIXEIRA_JSON_H */







