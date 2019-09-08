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

enum JsonType {
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_INTEGER,
	JSON_DOUBLE,
	JSON_STRING
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

enum JsonParseStatus {
	JSON_PARSE_OK = 0,
	JSON_PARSE_UNEXPECTED_END = -1,
	JSON_PARSE_ILLEGAL_CHAR = -2
};

enum JsonParseStatus parseJson(const char *const input, struct JsonNode **node);
size_t getDataLength(const char *const input);
enum JsonParseStatus parseRoot(const char *const input, char **node);
enum JsonParseStatus findObjectLength(const char *const input, unsigned int curIndex, size_t *outLength);
enum JsonParseStatus parseId(const char *const input, char **node, unsigned int *curIndex);
enum JsonParseStatus parseObject(const char *const input, char **node, unsigned int *curIndex);
enum JsonParseStatus parseString(const char *const input, char **node, unsigned int *curIndex);
enum JsonParseStatus findArrayLength(const char *const input, unsigned int curIndex, size_t *outLength);
enum JsonParseStatus parseArray(const char *const input, char **node, unsigned int *curIndex);
enum JsonParseStatus parseNode(const char *const input, char **node, unsigned int *curIndex);
enum JsonParseStatus parseNumeric(const char *const input, char **node, unsigned int *curIndex);
double stringToDouble(const char *const input, unsigned int start, unsigned int end);
double powerOf(double num, int exp);
enum JsonParseStatus parseExponent(const char *const input, char **node, unsigned int *curIndex, double num);

enum JsonParseStatus parseJson(const char *const input, struct JsonNode **node)
{
	size_t dataLength = getDataLength(input);
	void *data = malloc(sizeof(size_t) * (dataLength) * 2);

	void *another = data;
	enum JsonParseStatus status = parseRoot(input, (char**)&data);
	
	*node = another;
	return status;
	
}

size_t getDataLength(const char *const input)
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


enum JsonParseStatus parseRoot(const char *const input, char **node)
{
	unsigned int curIndex = 0;
	char curChar;
	while ((curChar = input[curIndex]) != '\0') {
		if (curChar == '{') {
			return parseObject(input, node, &curIndex);			
		} else if (curChar == '[') {
			return parseArray(input, node, &curIndex);
		} else if (!IS_WHITESPACE(curChar)) {
			return JSON_PARSE_ILLEGAL_CHAR;
		}

		curChar++;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus findObjectLength(const char *const input, unsigned int curIndex, size_t *outLength)
{
	size_t length = 0;
	char curChar;
	unsigned int level = 0;
	int inQuotes = 0;
	int isEscaped = 0;
	curIndex++;
	while ((curChar = input[curIndex]) != '\0') {

		if (inQuotes) {
			switch (curChar) {
			case '"':
				if (isEscaped) {
					isEscaped = !isEscaped;
				} else {
					inQuotes = !inQuotes;
				}
				break;
			case '\\':
				isEscaped = isEscaped ? 0 : 1;
				break;
			case '\0':
				return JSON_PARSE_UNEXPECTED_END;
			}
		} else {
			switch (curChar) {
			case '"':
				inQuotes = 1;
				break;
			case '}':
				if (level == 0) {
					*outLength = length;
					return JSON_PARSE_OK;
				}
			case ']':
				level--;
				break;
			case '{':
			case '[':
				level++;
				break;
			case ':':
				length += level == 0 ? 1 : 0; 
				break;
			case '\n':
			case '\t':
			case '\r':
			case ' ':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
			case 'e':
			case 'E':
			case '-':
			case '+':
			case ',':
				break;
			default:
				return JSON_PARSE_ILLEGAL_CHAR;
			}
		}

		curIndex++;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus parseId(const char *const input, char **node, unsigned int *curIndex)
{
	char curChar;
	int isEscaped = 0;
	while ((curChar = input[*curIndex]) != '\0') {
		switch (curChar) {
		case '"':
			(*curIndex)++;
			unsigned int startIndex = *curIndex;
			while (1) {
				curChar = input[*curIndex];
				switch (curChar) {
				case '"':
					if (isEscaped) {
						isEscaped = !isEscaped;
					} else {
						size_t stringSize = *curIndex - startIndex;
						if (stringSize > 0) {
							memcpy(*node, input + startIndex, stringSize);
							*node += stringSize;
						}

						*(char*)*node = '\0';
						(*node)++;
						*curIndex += 1;
						return JSON_PARSE_OK;
					}
					break;
				case '\\':
					isEscaped = isEscaped ? 0 : 1;
					break;
				}

				(*curIndex)++;
			}

			break;
		case '\n':
		case '\t':
		case '\r':
		case ' ':
			break;
		default:
			return JSON_PARSE_ILLEGAL_CHAR;
		}
		
		(*curIndex)++;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus parseObject(const char *const input, char **node, unsigned int *curIndex)
{
	*(enum JsonType*)*node = JSON_OBJECT;
	*node += sizeof(enum JsonType);
	char *nodeStart = *node;
	
	enum JsonParseStatus status;
	size_t numEntries;

	if ((status = findObjectLength(input, *curIndex, &numEntries)) < 0) {
		return status;
	}
	
	*(size_t*)*node = numEntries;
	*node += sizeof(size_t);

	size_t *idOffsets = (size_t*)*node;
	*node += sizeof(size_t) * numEntries;
	
	char curChar;
	(*curIndex)++;
	for (unsigned int i = 0; i < numEntries; i++)
	{
		idOffsets[i] = *node - nodeStart;
		/* parses and saves the entry id */
		if ((status = parseId(input, node, curIndex)) < 0) {
			return status;
		}
		
		/* finds the ':' char */
		while (1) {
			curChar = input[*curIndex];
			
			if (curChar == ':') {
				(*curIndex)++;
				break;
			} else if (!IS_WHITESPACE(curChar)) {
				return JSON_PARSE_ILLEGAL_CHAR;
			} else if (curChar == '\0') {
				return JSON_PARSE_UNEXPECTED_END;
			}

			(*curIndex)++;
		}

		/* parses the entry */
		if ((status = parseNode(input, node, curIndex)) < 0) {
			return status;
		}

		/* finds if there's a next entry or not. */
		while (1) {
			curChar = input[*curIndex];

			if (curChar == '}') {
				(*curIndex)++;
				return JSON_PARSE_OK;
			} else if (curChar == ',') {
				break;
			} else if (curChar == '\0') {
				return JSON_PARSE_UNEXPECTED_END;
			}

			(*curIndex)++;
		}

		(*curIndex)++;
	}

	return JSON_PARSE_OK;
}

enum JsonParseStatus parseString(const char *const input, char **node, unsigned int *curIndex)
{
	*(enum JsonParseStatus*)*node = JSON_STRING;
	*node += sizeof(enum JsonParseStatus);

	char curChar;
	int isEscaped = 0;
	(*curIndex)++;
	unsigned int startIndex = *curIndex;
	while ((curChar = input[*curIndex]) != '\0') {
		
		switch (curChar) {
		case '"':
			if (isEscaped) {
				isEscaped = !isEscaped;
			} else {
				size_t stringSize = *curIndex - startIndex;
				if (stringSize > 0) {
					memcpy(*node, input + startIndex, stringSize);
					*node += stringSize;
				}

				*(char*)*node = '\0';
				(*node)++;
				*curIndex += 1;
				return JSON_PARSE_OK;
			}
			break;
		case '\\':
			isEscaped = isEscaped ? 0 : 1;
			break;
		}
		
		(*curIndex)++;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus findArrayLength(const char *const input, unsigned int curIndex, size_t *outLength)
{
	size_t length = 0;
	char curChar;
	unsigned int level = 0;
	int inQuotes = 0;
	int isEscaped = 0;
	int inNumber = 0;
	while ((curChar = input[curIndex]) != '\0') {
		if (inQuotes) {
			switch (curChar) {
			case '"':
				if (isEscaped) {
					isEscaped = !isEscaped;
				} else {
					inQuotes = !inQuotes;
					level--;
				}
				break;
			case '\\':
				isEscaped = isEscaped ? 0 : 1;
				break;
			}
		} else {
			if (inNumber) {
				switch (curChar) {
				case ']':
				case '}':
				case ',':
				case '\n':
				case '\t':
				case '\r':
				case ' ':
					curIndex--;
					inNumber = 0;
					break;
				case '.':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					break;
				default:
					return JSON_PARSE_ILLEGAL_CHAR;
				}
			} else {
				switch (curChar) {
				case '"':
					inQuotes = 1;
				case '[':
				case '{':
					length += level == 0 ? 1 : 0;
					level++;
					break;
				case '-':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					length += level == 0 ? 1 : 0;
					inNumber = 1;
					break;
				case ']':
					if (level == 0) {
						*outLength = length;
						return JSON_PARSE_OK;
					}
				case '}':
					level--;
					break;
				case ',':
				case '\n':
				case '\r':
				case '\t':
				case ' ':
					break;
				default:
					return JSON_PARSE_ILLEGAL_CHAR;
				}				
			}

		}
		
		curIndex++;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

enum JsonParseStatus parseArray(const char *const input, char **node, unsigned int *curIndex)
{
	char *nodeStart = *node;
	
	/* sets the node type */
	*(enum JsonType*)*node = JSON_ARRAY;
	*node += sizeof(enum JsonType);

	enum JsonParseStatus status;

	/*
	size_t length;
	(*curIndex)++;
	if ((status = findArrayLength(input, *curIndex, &length)) < 0) {
		return status;
	}
	
 	*(size_t*)(*node) = length;
	(*node) += sizeof(size_t);
	*/
	size_t *offsets = (size_t*)*node;
	*node = (char*)(offsets + length);
	
	unsigned int numParsedNodes = 0;

	char curChar;
	while (numParsedNodes < length) {
		curChar = input[*curIndex];

		offsets[numParsedNodes] = *node - nodeStart;
		if ((status = parseNode(input, node, curIndex)) < 0) {
			return status;
		}


		numParsedNodes++;
		curChar = input[*curIndex];
		int shouldEnd = 0;
		while (!shouldEnd) {
			curChar = input[*curIndex];
			switch (curChar) {
			case ']':
				(*curIndex)++;
				return JSON_PARSE_OK;
			case ',':
				shouldEnd = 1;
				break;
			case '\0':
				return JSON_PARSE_UNEXPECTED_END;
			case '\n':
			case '\t':
			case '\r':
			case ' ':
				break;
			default:
				return JSON_PARSE_ILLEGAL_CHAR;
			}

			(*curIndex)++;
		}
	}

	(*curIndex)++;
	return JSON_PARSE_OK;
}

enum JsonParseStatus parseNode(const char *const input, char **node, unsigned int *curIndex)
{
	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {

		if (curChar == '{') {
			return parseObject(input, node, curIndex);
		} else if (curChar == '[') {
			return parseArray(input, node, curIndex);
		} else if (IS_NUMERIC(curChar)) {
			return parseNumeric(input, node, curIndex);
		} else if (curChar == '"') {
			return parseString(input, node, curIndex);
		} else if (!IS_WHITESPACE(curChar)) {
			return JSON_PARSE_ILLEGAL_CHAR;
		}

		(*curIndex)++;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

double stringToDouble(const char *const input, unsigned int start, unsigned int end)
{
	int sign = 1;
	if (input[start] == '-') {
		sign = -1;
		start++;
	} else if (input [start] == '+') {
		start++;
	}

	double num = 0;
	switch (end - start) {
	case 10: num += ((double)(input[start++]) - UTF8_ZERO) * 10000000000;
	case 9: num += ((double)(input[start++]) - UTF8_ZERO) * 1000000000;
	case 8: num += ((double)(input[start++]) - UTF8_ZERO) * 100000000;
	case 7: num += ((double)(input[start++]) - UTF8_ZERO) * 10000000;
	case 6: num += ((double)(input[start++]) - UTF8_ZERO) * 1000000;
	case 5: num += ((double)(input[start++]) - UTF8_ZERO) * 100000;
	case 4: num += ((double)(input[start++]) - UTF8_ZERO) * 10000;
	case 3: num += ((double)(input[start++]) - UTF8_ZERO) * 1000;
	case 2: num += ((double)(input[start++]) - UTF8_ZERO) * 100;
	case 1: num += ((double)(input[start++]) - UTF8_ZERO) * 10;
	case 0: num += ((double)(input[start]) - UTF8_ZERO);
	}

	return num * sign;
}

double powerOf(double num, int exp)
{
	double result = 1;
	double absolute = exp < 0 ? -exp : exp;
	if (exp >= 0) {
		for (int i = 0; i < absolute; i++) {
			result *= num;
		}
	} else {
		for (int i = 0; i < absolute; i++) {
			result /= num;
		}
	}


	return result;
}

enum JsonParseStatus parseNumeric(const char *const input, char **node, unsigned int *curIndex)
{
	unsigned int startIndex = *curIndex;
 
	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {
        
		switch (curChar) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case '.':;
			double integerPart = stringToDouble(input, startIndex, *curIndex - 1);
			*curIndex += 1;
			startIndex = *curIndex;
			while (1) {
				curChar = input[*curIndex];
				switch (curChar) {
				case '\0':
					return JSON_PARSE_UNEXPECTED_END;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					break;
				case 'e':
				case 'E':;
					double partialFraction = stringToDouble(input, startIndex, *curIndex - 1);
					double partialResult = integerPart + (partialFraction / powerOf(10, *curIndex - startIndex));
					*curIndex += 1;
					return parseExponent(input, node, curIndex, partialResult);
				default:;
					double fractionPart = stringToDouble(input, startIndex, *curIndex - 1);
					double result = integerPart + (fractionPart / powerOf(10, *curIndex - startIndex));
					*(enum JsonType*)*node = JSON_DOUBLE;
					*node += sizeof(enum JsonType);
					memcpy(*node, &result, sizeof(double));
					*node += sizeof(double);
					return JSON_PARSE_OK;
				}
                    
				*curIndex += 1;
			}
                
                case 'e':
		case 'E':;
			double partialResult = stringToDouble(input, startIndex, *curIndex - 1);
			*curIndex += 1;
			return parseExponent(input, node, curIndex, partialResult);
		default:;
			double result = stringToDouble(input, startIndex, *curIndex - 1);
			*(enum JsonType*)*node = JSON_DOUBLE;
			*node += sizeof(enum JsonType);
			memcpy(*node, &result, sizeof(double));
			*node += sizeof(double);
			return JSON_PARSE_OK;
		}
        
		*curIndex += 1;
	}

	return JSON_PARSE_UNEXPECTED_END;
    
}

enum JsonParseStatus parseExponent(const char *const input, char **node, unsigned int *curIndex, double num)
{
	char curChar;
	unsigned int startIndex = *curIndex;
	while ((curChar = input[*curIndex]) != '\0') {
		switch (curChar) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '+':
		case '-':
			break;
		default:;
			double exp = stringToDouble(input, startIndex, *curIndex - 1);
			*(enum JsonType*)*node = JSON_DOUBLE;
			*node += sizeof(enum JsonType);
			exp = powerOf(10, exp);
			num *= exp;
			memcpy(*node, &num, sizeof(double));
			*node += sizeof(double);

			return JSON_PARSE_OK;
		}
		
		*curIndex += 1;
	}

	return JSON_PARSE_UNEXPECTED_END;
}

/* printing */
#include <stdio.h>
void printJson(struct JsonNode *node);
void printObject(struct JsonObject *object);
void printArray(struct JsonArray *array);
void printString(char *node);
void printInteger(int num);
void printDouble(double num);

void printJson(struct JsonNode *node)
{
	switch (node->type) {
	case JSON_OBJECT:
		printObject((struct JsonObject*)node->data);
		break;
	case JSON_ARRAY:
		printArray((struct JsonArray*)node->data);
		break;
	case JSON_INTEGER:
		printInteger(*(int*)node->data);
		break;
	case JSON_DOUBLE:
		printDouble(*(double*)node->data);
		break;
	case JSON_STRING:
		printString(node->data);
		break;
	}
}

void printObject(struct JsonObject *object)
{
	printf("{");
	size_t numEntries = object->numNodes;
	size_t *offsets = (size_t*)object->data;
	
	for (size_t i = 0; i < numEntries; i++) {
		char *id = (char*)object + offsets[i];
		char *data = id + ((strlen(id) + 1) * sizeof(char));
		printf("\"%s\" : ", id);
		printJson((struct JsonNode*)data);

		if (i != numEntries - 1) {
			printf(", \n");	
		}
	}

	printf("\n}\n");
}

void printArray(struct JsonArray *array)
{
	printf("[");
	size_t numNodes = array->numNodes;
	size_t *offsets = (size_t*)array->data;
	
	for (size_t i = 0; i < numNodes; i++) {

		struct JsonNode *node = (struct JsonNode*)((char*)array + offsets[i] - sizeof(enum JsonType));
		printJson(node);
		
		if (i != numNodes - 1) {
			printf(", \n");	
		}
	}

	printf("]\n");
	
}

void printString(char *node)
{
	printf("\"%s\"", (char*)node);
}

void printInteger(int num)
{
	printf("%d", num);
}

void printDouble(double num)
{
	printf("%f", num);
}

#endif /* GMTEIXEIRA_JSON_H */







