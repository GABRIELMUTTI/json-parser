#ifndef GMTEIXEIRA_JSON_H
#define GMTEIXEIRA_JSON_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define IS_NUMERIC(c) (c > 0x29 && c < 0x3a)

enum JsonType {
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_INTEGER,
	JSON_FLOAT,
	JSON_STRING
};

struct JsonNode {
	enum JsonType type;
	char data[]; /* \0 indicates end of data for objects and arrays */
};


int gmParseJson(const char *const input, struct JsonNode **jsonNode);
int parseNode(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode);
int parseObject(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode);
int parseArray(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode);
int parseNumeric(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode);
int parseDouble(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode, unsigned int startIndex);
int parseString(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode);

int gmParseJson(const char *const input, struct JsonNode **jsonNode)
{
	size_t length = strlen(input);
	void *data = malloc(sizeof(char*) * length);
	unsigned int curIndex = 0;

	*jsonNode = data;
    
	return parseNode(input, &curIndex, &data);
}

int parseNode(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode)
{
	char curChar = input[*curIndex];
	(*curIndex)++;
	if (curChar == '{') {
		return parseObject(input, curIndex, jsonNode);
	} else if (curChar == '[') {
		return parseArray(input, curIndex, jsonNode);
	} else if (curChar == '"') {
		return parseString(input, curIndex, jsonNode);
	} else if (IS_NUMERIC(curChar)) {
		return parseNumeric(input, curIndex, jsonNode);
	}

	return -1;
}

int parseObject(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode)
{
	*((char*)*jsonNode) = JSON_OBJECT;
	(*jsonNode)++;

	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {

		if (curChar == '"') {
			if (parseId(input, curIndex, jsonNode) < 0) {
				return -1;
			}	
		}
		
		while ((curChar = input[*curIndex]) != '\0') {
			if (curChar == ':') {
				if (parseNode(input, curIndex, jsonNode) < 0) {
					break;
				}
			}
		}

		if (curChar == '}') {
			return 0;
		}

		(*curIndex)++;
	}
	
	return -1;
}

int parseArray(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode)
{
	*((char*)*jsonNode) = JSON_ARRAY;
	(*jsonNode)++;

	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {
		if (curChar == ']') {
			*(char*)*jsonNode = '\0';
			(*jsonNode)++;
			return 0;
			
		} else if (IS_NUMERIC(curChar)) {
			if (parseNumeric(input, curIndex, jsonNode) < 0) {
				return -1;
			}
			
		} else if (curChar == '{') {
			if (parseObject(input, curIndex, jsonNode) < 0) {
				return -1;
			}
			
		} else if (curChar == '[') {
			if (parseArray(input, curIndex, jsonNode) < 0) {
				return -1;
			}
			
		} else if (curChar == '"') {
			if (parseString(input, curIndex, jsonNode) < 0) {
				return -1;
			}
		}

		(*curIndex)++;
	}
	
	return -1;
}

int parseNumeric(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode)
{	
	unsigned int startIndex = *curIndex;
	
	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {
	
		if (curChar == '.') {
			return parseDouble(input, curIndex, jsonNode, startIndex);
		}

		if (!IS_NUMERIC(curChar)) {
			int32_t num;
			size_t byteSize;
			stringToInteger(input, startIndex, *curIndex, &num, &byteSize);

			*((char*)*jsonNode) = JSON_INTEGER;
			memcpy(*(jsonNode + 1), &num, sizeof(char) * byteSize);
			(*jsonNode) += byteSize;
			return 0;
		}
	
		(*curIndex)++;
	}

	return -1;
}

int parseDouble(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode, unsigned int startIndex)
{	
	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {

		if (!IS_NUMERIC(curChar)) {
			double num;
			size_t byteSize;
			stringToDouble(input, &num, startIndex, curIndex, &byteSize);

			*((char*)*jsonNode) = JSON_FLOAT;
			memcpy(*(jsonNode + 1), &num, sizeof(char) * byteSize);
			(*jsonNode) += byteSize;
			return 0;
		}

		(*curIndex)++;
	}

	return -1;
}

int parseString(const char *const input, unsigned int *curIndex, struct JsonNode **jsonNode)
{
	unsigned int startIndex = *curIndex;
	
	char curChar;
	while ((curChar = input[*curIndex]) != '\0') {
		if (curChar == '"') {
			size_t byteSize = *curIndex - startIndex;
			memcpy(*jsonNode, input, sizeof(char) * byteSize);
			*((char*)(*jsonNode) + byteSize + 1) = '\0';

			(*jsonNode) += byteSize + 1;
			return 0;
		}

		(*curIndex)++;
	}

	return -1;
}

#endif /* GMTEIXEIRA_JSON_H */
