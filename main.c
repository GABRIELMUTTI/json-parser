#include "cjson.h"
#include <stdio.h>

// ]},
int main(int argc, char *argv[]) {
//[\"okdasds{}\", [1, 2 , /"/"/"/", []]


	char *buffer = NULL;
	long length;
	printf("file: %s\n", argv[1]);
	FILE *f = fopen(argv[1], "rb");

	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length + 1);
		if (buffer)
		{
			fread(buffer, 1, length, f);
		}
		buffer[length] = '\0';
		fclose(f);
	}

//	printf("json: %s\n", buffer);

	int status;
	for (int i = 0; i < atoi(argv[2]); i++) {
		char *data;		
		status = parseJson(buffer, (struct JsonNode**)&data);
//		free(data);
	}	
	free(buffer);
//	/*
	switch (status) {
	case JSON_PARSE_OK:
		printf("PARSE OK!\n");
//		printJson((struct JsonNode*)data, 0);
		break;
	case JSON_PARSE_ILLEGAL_CHAR:
		printf("ILLEGAL CHAR!\n");
		break;
	case JSON_PARSE_UNEXPECTED_END:
		printf("UNEXPECTED END!\n");
		break;
	default:
		printf("Something weird happened.\n");
	}

//	free(data);
//	*/
	return 0;
}

/*

30 -> 4000
25 -> x

30x = 4000 * 25
x = 4000 * 25 / 30


5 days = 667

 */
