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

#ifdef SHL_TOKEN_IMPLEMENTATION

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SHL_TOKEN_DEBUG
	#define SHL_TOKEN_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
	#define SHL_TOKEN_DEBUG_PRINT(...)
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

shl_tokens shl_token_tokenise(const char* tokenise, shl_token_opts* opts) {
	if(!tokenise || !opts) {
		return (shl_tokens){ NULL, NULL };
	}
	size_t oryginalLen = opts->size;
	size_t separatorCount = 0;
	size_t tokensCount = 0;
	(void)separatorCount;

	{
		size_t i = 0;
		const char* ptrr = tokenise;

		bool inToken = false;
		bool quouted = false;

		while(opts->size == 0 ? *ptrr != opts->nullTerminatorIn : i < opts->size) {
			if(opts->quouteEscape && *ptrr == opts->quouteEscapeChar) {
				if(i + 1 < oryginalLen || opts->size == 0) {
					if(*(ptrr + 1) == opts->quetesChar || *(ptrr + 1) == opts->quouteEscapeChar) {
						i++;
						ptrr++;
						goto shl_tokenise_loop_end;
					}
				}
			}

			if(opts->quotes && *ptrr == opts->quetesChar) {
				if(!inToken) {
					inToken = true;
					tokensCount++;
				}
				
				quouted = !quouted;

				goto shl_tokenise_loop_end;
			}

			if(!quouted && *ptrr == opts->separator) {
				inToken = false;

				separatorCount++;
				goto shl_tokenise_loop_end;
			}

			if(!inToken) {
				inToken = true;
				tokensCount++;
			}

shl_tokenise_loop_end:
			SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] i=%zu char='%c' inToken=%d quoted=%d\n", i, *ptrr, inToken, quouted);
			i++;
			ptrr += sizeof(char);
		}
		if(oryginalLen == 0) { oryginalLen = i; }
	}
	size_t oryginalSize = oryginalLen + 1;

	SHL_TOKEN_DEBUG_PRINT("\n\n\n");

	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] Scanned string to tokenise:\n");
	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] tokenise: {%s}\n", tokenise);
	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] \noryginalLen: %zu\n", oryginalLen);
	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] oryginalSize: %zu\n", oryginalSize);
	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] \nseparatorCount: %zu\n", separatorCount);
	SHL_TOKEN_DEBUG_PRINT("[SHL_TOKEN_DEBUG] tokensCount: %zu\n", tokensCount);

	SHL_TOKEN_DEBUG_PRINT("\n\n\n");

	// + nullTerm
	size_t tokensSize = (oryginalSize * sizeof(char)) + (tokensCount * sizeof(char));
	// + tokensCount
	size_t metadataSize = (tokensCount + 1) * sizeof(size_t);
	size_t tokPtrSize = tokensCount * sizeof(char*);

	if(metadataSize > SIZE_MAX - tokPtrSize || metadataSize + tokPtrSize > SIZE_MAX - tokensSize) {
		return (shl_tokens){ NULL, NULL };
	}

	// meta -> tokensPtrs -> tokens
	void* allocation = malloc(metadataSize + tokPtrSize + tokensSize);
	if(!allocation) { return (shl_tokens){ NULL, NULL }; }

	size_t* metaTokens = allocation;
	char** tokPtrrs = (char**)((char*)allocation + metadataSize);
	char* tokenst = (char*)((char*)allocation + metadataSize + tokPtrSize);

	metaTokens[0] = tokensCount;

	{
		size_t currCount = 0;
		size_t tokenOffset = 0;
		size_t tokenLen = 0;
		bool inToken = false;
		bool quouted = false;
		for(size_t i = 0; i < oryginalLen; i++) {
			char c = tokenise[i];

			if(opts->quotes && c == opts->quetesChar) {
				if(!inToken) {
					tokPtrrs[currCount] = tokenst + tokenOffset;
					inToken = true;
				}

				quouted = !quouted;

				if(!opts->keepQuotes) {
					continue;
				}
			}

			if(opts->quouteEscape && c == opts->quouteEscapeChar && i + 1 < oryginalLen) {
				if(tokenise[i + 1] == opts->quetesChar || tokenise[i + 1] == opts->quouteEscapeChar) {
					i++;
					c = tokenise[i];
				}
			}

			if(!quouted && c == opts->separator) {
				if(inToken) {
					tokenst[tokenOffset++] = opts->nullTerminatorOut;
					metaTokens[currCount + 1] = tokenLen;
					currCount++;
					inToken = false;
					tokenLen = 0;
				}
				continue;
			}
	
			if(!inToken) {
				tokPtrrs[currCount] = tokenst + tokenOffset;
				inToken = true;
			}
			tokenst[tokenOffset++] = c;
			tokenLen++;
		}

		if(inToken) {
			tokenst[tokenOffset++] = opts->nullTerminatorOut;
			metaTokens[currCount + 1] = tokenLen;
			currCount++;
		}

	}

	return (shl_tokens){ tokPtrrs, metaTokens };
}

shl_tokens shl_token_ftokenise(FILE* file, const shl_token_opts* opts) {
	if(!file || !opts) {
		return (shl_tokens){ NULL, NULL };
	}

	size_t capacity = 64 * 1024;
	size_t size = 0;

	char* buffer = malloc(capacity + 1);
	if(!buffer) {
		return (shl_tokens){ NULL, NULL };
	}

	while(true) {
		if(size == capacity) {
			if(capacity >= SIZE_MAX / 2) {
				free(buffer);
				return (shl_tokens){ NULL, NULL };
			}

			size_t newCapacity = capacity * 2;
			char* tmp = realloc(buffer, newCapacity + 1);
			if(!tmp) {
				free(buffer);
				return (shl_tokens){ NULL, NULL };
			}

			buffer = tmp;
			capacity = newCapacity;
		}

		size_t requested = capacity - size;
		size_t read = fread(buffer + size, 1, requested, file);
		size += read;

		if(ferror(file)) {
			free(buffer);
			return (shl_tokens){ NULL, NULL };
		}

		if(read < requested) {
			break;
		}
	}

	buffer[size] = '\0';

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
