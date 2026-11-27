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

typedef struct shl_string {
	char* data;
	const size_t* length;
	const size_t* capacity;
} shl_string;

shl_string shl_string_create(const char* contents, size_t* size);
void shl_string_free(shl_string string);
shl_string shl_string_strdup(shl_string string, bool cap_is_len);

#ifdef __cplusplus
}
#endif

#ifdef SHL_STRING_IMPLEMENTATION

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static shl_string shl_string_empty(void) {
	shl_string empty = { NULL, NULL, NULL };
	return empty;
}

// Not including null term
shl_string shl_string_create(const char* contents, size_t* size) {
	if(!size && !contents) { errno = EINVAL; return shl_string_empty(); }
	shl_string string;
	
	size_t strerSize = size ? *size : strlen(contents) * sizeof(char);
	size_t strerLen = strerSize / sizeof(char);

	if(strerSize > SIZE_MAX - (2 * sizeof(size_t)) - sizeof(char)) {
		errno = EOVERFLOW;
		return shl_string_empty();
	}

	// + null
	size_t* alloc = (size_t*)malloc(strerSize + (2 * sizeof(size_t)) + sizeof(char));
	if(!alloc) { return shl_string_empty(); }
	string.data = (char*)alloc + (2 * sizeof(size_t));

	alloc[0] = strerLen; string.capacity = &alloc[0];
	alloc[1] = strerLen; string.length = &alloc[1];

	for(size_t i = 0; i < strerLen; i++) {
		string.data[i] = contents ? contents[i] : '\0';
	}
	string.data[strerLen] = '\0';

	return string;
}

void shl_string_free(shl_string string) {
	free((size_t*)string.capacity);
}

shl_string shl_string_strdup(shl_string string, bool cap_is_len) {
	if(!string.data || !string.capacity || !string.length || *string.length > *string.capacity) { errno = EINVAL; return shl_string_empty(); }
	size_t capacity = cap_is_len ? *string.length : *string.capacity;
	if(capacity > SIZE_MAX - (2 * sizeof(size_t)) - sizeof(char)) {
		errno = EOVERFLOW;
		return shl_string_empty();
	}
	size_t* alloc = (size_t*)malloc(capacity + (2 * sizeof(size_t)) + sizeof(char));
	if(!alloc) { return shl_string_empty(); }
	char* alloStr = (char*)alloc + (2 * sizeof(size_t));

	alloc[0] = capacity;
	alloc[1] = *string.length;

	for(size_t i = 0; i <= *string.length; i++) {
		alloStr[i] = string.data[i];
	}

	shl_string result;
	result.data = alloStr;
	result.length = &alloc[1];
	result.capacity = alloc;
	return result;
}

#endif

#endif
