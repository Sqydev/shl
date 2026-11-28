/* 
* Copyright (c) 2025-present W.K. ( _Sqyd_ / Sqydev )
* Github: https://github.com/Sqydev
* GPG Fingerprint: 6DC2516B0DFDA9C59661650722F7B8A777F33B56
*
* This software is provided "as-is", without any express or implied warranty. In no event
* will the authors be held liable for any damages arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose, including commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
* 1. Non-Misrepresentation: The origin of this software must not be misrepresented; 
*    you must not claim that you wrote the original software. An acknowledgment in 
*    product documentation is appreciated but not required.
*
* 2. Source-Level Copyleft: Any altered versions (forks) of this software's source code, 
*    or files containing significant portions of this code, must be distributed under 
*    these same license terms. Such modified source code must be made publicly available 
*    to any recipient, even if used over a network (SaaS).
* 
* 3. Proprietary Integration: This software may be integrated into, linked with, or 
*    used as a component of proprietary and closed-source products. In such cases, 
*    the surrounding proprietary application code does not need to be disclosed, 
*    provided that the original or modified source code of THIS software remains 
*    available under the terms of Section 2.
*
* 4. Persistent Metadata: All original credits, including those in the source code headers 
*    and binary metadata (e.g., ELF .comment section, PE StringFileInfo, or equivalent), 
*    must not be removed. You may add your own credits to forks, provided the original 
*    authorship remains clearly identified.
* 
* 5. Notice Retention: This license notice may not be removed or altered from any 
*    source or binary distribution.
*/

#ifndef SHL_STRING_INCLUDED
#define SHL_STRING_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Growth strategies used when a string needs more capacity.
typedef enum shl_string_growth {
	SHL_STRING_GROW_LEAP,
	SHL_STRING_GROW_STEP,
} shl_string_growth;

typedef struct shl_string {
	char* data;
	const size_t* length;
	const size_t* capacity;

	shl_string_growth growth;
} shl_string;

shl_string shl_string_create(const char* contents, size_t* size);
void shl_string_free(shl_string* string);

shl_string shl_string_strdup(shl_string string, bool cap_is_len);

// Ensures room for at least capacity bytes.
bool shl_string_reserve(shl_string* string, size_t capacity);
// Shrinking capacity to the current length.
bool shl_string_tighten(shl_string* string);
// Sets the length to zero, keeping the capacity.
bool shl_string_clear(shl_string* string);

// Append csting to shl_string
bool shl_string_append(shl_string* string, const char* text, shl_string_growth* growth);
// Append char to shl_string
bool shl_string_append_char(shl_string* string, char character, shl_string_growth* growth);
// Append shl_string to shl_string
bool shl_string_appends(shl_string* string, shl_string source, shl_string_growth* growth);

// Insert cstring to index
bool shl_string_splice(shl_string* string, size_t index, const char* text);
// Delete count chars from index
bool shl_string_scrub(shl_string* string, size_t index, size_t count);

// Strcmp but binary
bool shl_string_equals(shl_string string, const char* text);
// Strstr == NULL
bool shl_string_holds(shl_string string, const char* needle);

// Get alternative version of shl_string where shl_string doesn't exist :0
const char* shl_string_cstr(shl_string string);
// Fast strlen()(reads metadata)
bool shl_string_hollow(shl_string string);

// Strcpy
bool shl_string_set(shl_string* string, const char* text);
// Memmove + strncpy
bool shl_string_mend(shl_string* string, size_t index, size_t count, const char* text);
// Copy count chars from index
shl_string shl_string_carve(shl_string string, size_t index, size_t count);
// Return slice of string from start to end
shl_string shl_string_weld(shl_string string, size_t start, size_t end);

// Strcmp
int shl_string_compare(shl_string string, const char* text);
// Strcmp for two shl_strings
bool shl_string_compares(shl_string a, shl_string b);
// Strncmp from start
bool shl_string_begins(shl_string string, const char* prefix);
// Strcmp from end
bool shl_string_closes(shl_string string, const char* suffix);

// Strstr but index
size_t shl_string_seek(shl_string string, const char* needle);
// Count strstr hits
size_t shl_string_echoes(shl_string string, const char* needle);

// Toupper each char, new string
shl_string shl_string_raise(shl_string string);
// Tolower each char, new string
shl_string shl_string_lower(shl_string string);
// Strip whitespace from both ends
bool shl_string_trim(shl_string* string);

#ifdef __cplusplus
}
#endif

#ifdef SHL_STRING_IMPLEMENTATION

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static shl_string shl_string_empty(void) {
	shl_string empty = { NULL, NULL, NULL, SHL_STRING_GROW_LEAP };
	return empty;
}

shl_string shl_string_create(const char* contents, size_t* size) {
	if(!size && !contents) { errno = EINVAL; return shl_string_empty(); }
	shl_string string;
	
	size_t strerSize = size ? *size : strlen(contents) * sizeof(char);
	if(contents && size) {
		size_t available = strlen(contents);
		if(strerSize > available) { strerSize = available; }
	}
	size_t strerLen = strerSize / sizeof(char);

	if(strerSize > SIZE_MAX - (2 * sizeof(size_t)) - sizeof(char)) {
		errno = EOVERFLOW;
		return shl_string_empty();
	}

	size_t* alloc = malloc(strerSize + (2 * sizeof(size_t)) + sizeof(char));
	if(!alloc) { errno = ENOMEM; return shl_string_empty(); }
	string.data = (char*)alloc + (2 * sizeof(size_t));

	alloc[0] = strerLen; string.capacity = &alloc[0];
	alloc[1] = strerLen; string.length = &alloc[1];

	for(size_t i = 0; i < strerLen; i++) {
		string.data[i] = contents ? contents[i] : '\0';
	}
	string.data[strerLen] = '\0';

	string.growth = SHL_STRING_GROW_LEAP;

	return string;
}

void shl_string_free(shl_string* string) {
	if(!string || !string->capacity) { return; }
	free((void*)string->capacity);
	string->data = NULL;
	string->length = NULL;
	string->capacity = NULL;
	string->growth = SHL_STRING_GROW_LEAP;
}

shl_string shl_string_strdup(shl_string string, bool cap_is_len) {
	if(!string.data || !string.capacity || !string.length || *string.length > *string.capacity) { errno = EINVAL; return shl_string_empty(); }
	size_t capacity = cap_is_len ? *string.length : *string.capacity;
	if(capacity > SIZE_MAX - (2 * sizeof(size_t)) - sizeof(char)) {
		errno = EOVERFLOW;
		return shl_string_empty();
	}
	size_t* alloc = malloc(capacity + (2 * sizeof(size_t)) + sizeof(char));
	if(!alloc) { errno = ENOMEM; return shl_string_empty(); }
	char* alloStr = (char*)alloc + (2 * sizeof(size_t));
	memset(alloStr, '\0', capacity + 1);

	alloc[0] = capacity;
	alloc[1] = *string.length;

	for(size_t i = 0; i < *string.length; i++) {
		alloStr[i] = string.data[i];
	}
	alloStr[*string.length] = '\0';

	shl_string result;

	result.data = alloStr;
	result.length = &alloc[1];
	result.capacity = alloc;

	result.growth = string.growth;

	return result;
}

bool shl_string_reserve(shl_string* string, size_t capacity);
bool shl_string_tighten(shl_string* string);
bool shl_string_clear(shl_string* string);

bool shl_string_append(shl_string* string, const char* text, shl_string_growth* growth);
bool shl_string_append_char(shl_string* string, char character, shl_string_growth* growth);
bool shl_string_appends(shl_string* string, shl_string source, shl_string_growth* growth);

bool shl_string_splice(shl_string* string, size_t index, const char* text);
bool shl_string_scrub(shl_string* string, size_t index, size_t count);

bool shl_string_equals(shl_string string, const char* text);
bool shl_string_holds(shl_string string, const char* needle);

const char* shl_string_cstr(shl_string string);
bool shl_string_hollow(shl_string string);

bool shl_string_set(shl_string* string, const char* text);
bool shl_string_mend(shl_string* string, size_t index, size_t count, const char* text);
shl_string shl_string_carve(shl_string string, size_t index, size_t count);
shl_string shl_string_weld(shl_string string, size_t start, size_t end);

int shl_string_compare(shl_string string, const char* text);
bool shl_string_compares(shl_string a, shl_string b);
bool shl_string_begins(shl_string string, const char* prefix);
bool shl_string_closes(shl_string string, const char* suffix);

// Needle not found
#define SHL_STRING_NPOS ((size_t)-1)
size_t shl_string_seek(shl_string string, const char* needle);
size_t shl_string_echoes(shl_string string, const char* needle);

shl_string shl_string_raise(shl_string string);
shl_string shl_string_lower(shl_string string);
bool shl_string_trim(shl_string* string);

#endif

#endif
