#define SHL_STRING_IMPLEMENTATION
#include "shl_string.h"

#include <cstring>

int main() {
	shl_string string = shl_string_create("hello", nullptr);
	if(!string.data || !string.length || !string.capacity) { return 1; }
	if(*string.length != 5 || *string.capacity != 5) { return 1; }

	shl_string copy = shl_string_strdup(string, false);
	if(!copy.data || *copy.length != 5 || *copy.capacity != 5) { return 1; }
	if(std::memcmp(copy.data, "hello", 5) != 0) { return 1; }

	shl_string_free(copy);
	shl_string_free(string);
	return 0;
}
