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

#ifndef SHL_TOKEN_INCLUDED
#define SHL_TOKEN_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct shl_tokens {
	char** items;
	const size_t* count;
} shl_tokens;

typedef struct shl_token_opts {
	size_t size; // If 0 then the lib count BUT ONLY if it's null terminated, if not, there will be segv

	char nullTerminatorIn; // Null term, if you have to chainge it or something.
	char nullTerminatorOut; // The output will be terminated with this null term

	char separator;

	bool quotes;
	char quetesChar;
	bool quouteEscape; // The thing that safes "\""
	char quouteEscapeChar;
	bool keepQuotes;
} shl_token_opts;

shl_token_opts shl_token_get_defaults(void);
shl_tokens shl_token_tokenise(const char* tokenise, const shl_token_opts* opts);
shl_tokens shl_token_ftokenise(FILE* file, const shl_token_opts* opts);
void shl_token_free(shl_tokens tokens);
size_t shl_token_toklen(shl_tokens tokens, size_t index);

#ifdef __cplusplus
}
#endif

#ifdef SHL_TOKEN_IMPLEMENTATION

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef SHL_TOKEN_DEBUG
	#define SHL_TOKEN_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
	#define SHL_TOKEN_DEBUG_PRINT(...)
#endif

static shl_tokens shl_tokens_empty(void) {
	shl_tokens empty = { NULL, NULL };
	return empty;
}

shl_token_opts shl_token_get_defaults(void) {
	shl_token_opts opts;

	opts.size = 0;

	opts.nullTerminatorIn = '\0';
	opts.nullTerminatorOut = '\0';

	opts.separator = ' ';

	opts.quotes = true;
	opts.quetesChar = '"';
	opts.quouteEscape = true;
	opts.quouteEscapeChar = '\\';
	opts.keepQuotes = true;

	return opts;
}

typedef struct shl_token_state {
	const shl_token_opts* opts;
	size_t count;
	size_t tokenLen;
	size_t written;
	bool inToken;
	bool quoted;
} shl_token_state;

static void shl_token_begin(shl_token_state* state, char* out, char** ptrs) {
	if(state->inToken) { return; }
	if(ptrs) { ptrs[state->count] = out + state->written; }
	state->inToken = true;
}

static void shl_token_put(shl_token_state* state, char c, char* out) {
	if(out) { out[state->written] = c; }
	state->written++;
	state->tokenLen++;
}

static void shl_token_end(shl_token_state* state, char* out, size_t* lengths) {
	if(!state->inToken) { return; }
	if(out) { out[state->written] = state->opts->nullTerminatorOut; }
	state->written++;
	if(lengths) { lengths[state->count] = state->tokenLen; }
	state->count++;
	state->inToken = false;
	state->tokenLen = 0;
}

static size_t shl_token_step(shl_token_state* state, char c, bool hasNext, char next, char* out, char** ptrs, size_t* lengths) {
	const shl_token_opts* opts = state->opts;

	if(opts->quouteEscape && c == opts->quouteEscapeChar && hasNext && (next == opts->quetesChar || next == opts->quouteEscapeChar)) {
		if(!state->quoted && next == opts->separator) {
			shl_token_end(state, out, lengths);
		} else {
			shl_token_begin(state, out, ptrs);
			shl_token_put(state, next, out);
		}
		return 2;
	}

	if(opts->quotes && c == opts->quetesChar) {
		shl_token_begin(state, out, ptrs);
		state->quoted = !state->quoted;
		if(opts->keepQuotes) { shl_token_put(state, c, out); }
		return 1;
	}

	if(!state->quoted && c == opts->separator) {
		shl_token_end(state, out, lengths);
		return 1;
	}

	shl_token_begin(state, out, ptrs);
	shl_token_put(state, c, out);
	return 1;
}

shl_tokens shl_token_tokenise(const char* tokenise, const shl_token_opts* opts) {
	if(!tokenise || !opts) {
		return shl_tokens_empty();
	}

	size_t scanLen = opts->size;
	if(scanLen == 0) {
		const char* terminator = strchr(tokenise, opts->nullTerminatorIn);
		scanLen = terminator ? (size_t)(terminator - tokenise) : strlen(tokenise);
	} else if(opts->nullTerminatorIn != '\0') {
		const char* terminator = (const char*)memchr(tokenise, opts->nullTerminatorIn, scanLen);
		if(terminator) { scanLen = (size_t)(terminator - tokenise); }
	}

	shl_token_state countState = { opts, 0, 0, 0, false, false };
	size_t i = 0;
	while(i < scanLen) {
		bool hasNext = i + 1 < scanLen;
		i += shl_token_step(&countState, tokenise[i], hasNext, hasNext ? tokenise[i + 1] : '\0', NULL, NULL, NULL);
	}
	shl_token_end(&countState, NULL, NULL);

	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] scanned=%zu tokens=%zu bytes=%zu\n", scanLen, countState.count, countState.written);

	if(countState.count > (SIZE_MAX - sizeof(size_t)) / sizeof(size_t)) { return shl_tokens_empty(); }
	size_t metadataSize = (countState.count + 1) * sizeof(size_t);
	if(metadataSize > SIZE_MAX - countState.written) { return shl_tokens_empty(); }
	size_t dataOffset = metadataSize + countState.written;
	if(countState.count > (SIZE_MAX - dataOffset) / sizeof(char*)) { return shl_tokens_empty(); }
	size_t tokPtrSize = countState.count * sizeof(char*);
	size_t allocationSize = dataOffset + tokPtrSize;

	void* allocation = malloc(allocationSize);
	if(!allocation) { return shl_tokens_empty(); }

	size_t* metaTokens = (size_t*)allocation;
	char** tokPtrrs = (char**)((char*)allocation + metadataSize);
	char* tokenst = (char*)allocation + metadataSize + tokPtrSize;

	metaTokens[0] = countState.count;

	shl_token_state writeState = { opts, 0, 0, 0, false, false };
	i = 0;
	while(i < scanLen) {
		bool hasNext = i + 1 < scanLen;
		i += shl_token_step(&writeState, tokenise[i], hasNext, hasNext ? tokenise[i + 1] : '\0', tokenst, tokPtrrs, metaTokens + 1);
	}
	shl_token_end(&writeState, tokenst, metaTokens + 1);

	shl_tokens result;
	result.items = tokPtrrs;
	result.count = metaTokens;
	return result;
}

shl_tokens shl_token_ftokenise(FILE* file, const shl_token_opts* opts) {
	if(!file || !opts) {
		return shl_tokens_empty();
	}

	size_t limit = opts->size ? opts->size : SIZE_MAX;
	size_t capacity = 64 * 1024;
	if(capacity > limit) { capacity = limit; }
	size_t size = 0;

	char* buffer = (char*)malloc(capacity + 1);
	if(!buffer) {
		return shl_tokens_empty();
	}

	while(size < limit) {
		if(size == capacity) {
			if(capacity >= SIZE_MAX / 2) { break; }

			size_t newCapacity = capacity * 2;
			if(newCapacity > limit) { newCapacity = limit; }
			char* tmp = (char*)realloc(buffer, newCapacity + 1);
			if(!tmp) {
				free(buffer);
				return shl_tokens_empty();
			}

			buffer = tmp;
			capacity = newCapacity;
		}

		size_t requested = capacity - size;
		size_t read = fread(buffer + size, 1, requested, file);
		size += read;

		if(ferror(file)) {
			free(buffer);
			return shl_tokens_empty();
		}

		if(read < requested) {
			break;
		}
	}

	buffer[size] = '\0';

	const char* terminator = (const char*)memchr(buffer, opts->nullTerminatorIn, size);
	if(terminator) { size = (size_t)(terminator - buffer); }

	shl_token_opts fileOpts = *opts;
	fileOpts.size = size;

	shl_tokens tokens = shl_token_tokenise(buffer, &fileOpts);

	free(buffer);

	return tokens;
}

void shl_token_free(shl_tokens tokens) {
	free((void*)tokens.count);
}

// Faster, because it uses buffered informations.
size_t shl_token_toklen(shl_tokens tokens, size_t index) {
	if(!tokens.count || index >= tokens.count[0]) { return 0; }
	return tokens.count[index + 1];
}

#endif

#endif
