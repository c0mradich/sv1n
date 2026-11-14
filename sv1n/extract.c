#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* extractBuffer(char* buffer) {
	char* splitter = strchr(buffer, ' ');

	if (splitter == NULL)
		return NULL;

	splitter++; // пропускаем разделитель

	size_t len = strlen(splitter);

	char* extractedBuffer = malloc(len + 1);

	if (extractedBuffer == NULL)
		return NULL;

	strcpy_s(extractedBuffer, len + 1, splitter);

	return extractedBuffer;
}


char* messageForm(void) {
	size_t capacity = 64;
	size_t len = 0;

	char* message = malloc(capacity);

	if (message == NULL)
		return NULL;

	int c;

	while ((c = getchar()) != '\n' && c != EOF) {
		if (len + 1 >= capacity) {
			capacity *= 2;
			char* tmp = realloc(message, capacity);
			if (tmp == NULL)
			{
				free(message);
				return NULL;
			}
			message = tmp;
		}
		message[len++] = (char)c;
	}
	message[len] = '\0';
	return message;
}

char* extractFirstPart(char* buffer) {
	char* splitter = strchr(buffer, ' ');

	if (splitter == NULL)
		return NULL;

	size_t firstLength = splitter - buffer;

	char* primary = malloc(firstLength + 1);

	if (primary == NULL)
		return NULL;

	memcpy(primary, buffer, firstLength);
	primary[firstLength] = '\0';

	return primary;
}

char* execCommand(const char* command) {
	FILE* pipe = _popen(command, "r");

	if (pipe == NULL)
		return NULL;

	size_t capacity = 1024;
	size_t length = 0;

	char* result = malloc(capacity);

	if (result == NULL) {
		_pclose(pipe);
		return NULL;
	}

	char buffer[256];

	while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
		size_t chunkLength = strlen(buffer);

		if (length + chunkLength + 1 > capacity) {
			capacity *= 2;

			char* tmp = realloc(result, capacity);

			if (tmp == NULL) {
				free(result);
				_pclose(pipe);
				return NULL;
			}

			result = tmp;
		}

		memcpy(result + length, buffer, chunkLength);
		length += chunkLength;
	}

	result[length] = '\0';

	_pclose(pipe);

	return result;
}

char* defName(char* buffer) {
	char* pos = strchr(buffer, '>');

	if (pos != NULL) {
		size_t len = pos - buffer;

		char* username = malloc(len+1);

		memcpy(username, buffer, len);
		username[len] = '\0';

		return username;
	}
	return NULL;
}