#include "shl_string.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static size_t failures = 0;

#define SHL_CHECK(condition) do { \
	if(!(condition)) { \
		printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
		failures++; \
	} \
} while(0)

int main(void) {
	shl_string string = shl_string_create("hello", NULL);
	SHL_CHECK(string.data && string.length && string.capacity);
	SHL_CHECK(*string.length == 5 && *string.capacity == 5);
	SHL_CHECK(memcmp(string.data, "hello", 5) == 0);
	SHL_CHECK(string.data[5] == '\0');

	size_t tooLong = 100;
	shl_string clamped = shl_string_create("hi", &tooLong);
	SHL_CHECK(clamped.data && *clamped.length == 2 && *clamped.capacity == 2);
	SHL_CHECK(memcmp(clamped.data, "hi", 2) == 0);
	SHL_CHECK(clamped.data[2] == '\0');

	shl_string copy = shl_string_strdup(string, false);
	SHL_CHECK(copy.data && *copy.length == 5 && *copy.capacity == 5);
	SHL_CHECK(memcmp(copy.data, "hello", 5) == 0);
	SHL_CHECK(copy.data[5] == '\0');
	SHL_CHECK(copy.data != string.data);

	shl_string reduced = shl_string_strdup(string, false);
	((size_t*)reduced.length)[0] = 2;
	shl_string tailCopy = shl_string_strdup(reduced, false);
	SHL_CHECK(tailCopy.data && *tailCopy.capacity == 5);
	SHL_CHECK(tailCopy.data[2] == '\0' && tailCopy.data[5] == '\0');

	shl_string shorter = shl_string_strdup(string, true);
	SHL_CHECK(shorter.data && *shorter.length == 5 && *shorter.capacity == 5);
	SHL_CHECK(shorter.data[5] == '\0');

	shl_string_free(&shorter);
	shl_string_free(&tailCopy);
	shl_string_free(&reduced);
	shl_string_free(&copy);
	SHL_CHECK(copy.data == NULL && copy.length == NULL && copy.capacity == NULL);
	shl_string_free(&copy);

	size_t zero = 0;
	shl_string empty = shl_string_create(NULL, &zero);
	SHL_CHECK(empty.data && *empty.length == 0 && *empty.capacity == 0);
	SHL_CHECK(empty.data[0] == '\0');

	char raw[] = { 'a', 'b', 'c' };
	size_t rawLen = 3;
	size_t rawCap = 3;
	shl_string unterminated = { raw, &rawLen, &rawCap };
	shl_string fixed = shl_string_strdup(unterminated, false);
	SHL_CHECK(fixed.data && *fixed.length == 3);
	SHL_CHECK(memcmp(fixed.data, "abc", 3) == 0);
	SHL_CHECK(fixed.data[3] == '\0');
	shl_string_free(&fixed);

	errno = 0;
	shl_string invalid = shl_string_create(NULL, NULL);
	SHL_CHECK(!invalid.data && !invalid.length && !invalid.capacity);
	SHL_CHECK(errno == EINVAL);
	shl_string_free(&invalid);

	errno = 0;
	size_t huge = SIZE_MAX;
	shl_string overflow = shl_string_create(NULL, &huge);
	SHL_CHECK(!overflow.data && !overflow.length && !overflow.capacity);
	SHL_CHECK(errno == EOVERFLOW);
	shl_string_free(&overflow);

	shl_string_free(&string);
	shl_string_free(&clamped);
	shl_string_free(&empty);

	if(failures == 0) { printf("All shl_string tests passed.\n"); }
	return failures != 0;
}
