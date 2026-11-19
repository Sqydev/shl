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

#include <stdio.h>
#ifdef SHL_TOKEN_IMPLEMENTATION

#include <string.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct shl_tokens {
	char** items;
	const size_t* count;
} shl_tokens;

typedef struct shl_token_opts {
	size_t size; // If 0 then the lib count BUT ONLY if it's null terminated, if not, there will be segv
	char nullTerminatorIn; // Null term, if you have to chainge it or something.
	char nullTerminatorOut; // The output will be terminated with this null term
	char separator; // Token separator
} shl_token_opts;

shl_token_opts shl_get_default_opts(void) {
	shl_token_opts opts;

	opts.nullTerminatorIn = '\0';
	opts.nullTerminatorOut = '\0';
	opts.size = 0;
	opts.separator = ' ';

	return opts;
}

shl_tokens shl_tokenise(const char* tokenise, shl_token_opts* opts) {
	size_t oryginalLen = opts->size;
	size_t separatorCount = 0;
	size_t tokensCount = 1;

	{
		size_t i = 0;
		char* ptrr = (char*)tokenise;
		bool wasPreviousSep = false;
		bool wasThereAnyTokenBefore = false;
		while(opts->size == 0 ? *ptrr != opts->nullTerminatorIn : i < opts->size) {
			if(*ptrr == opts->separator) {
				separatorCount++;
				if(!wasPreviousSep && *(ptrr + sizeof(char)) != opts->nullTerminatorIn && *(ptrr + sizeof(char)) != opts->separator && wasThereAnyTokenBefore == true) {
					wasPreviousSep = true;
					tokensCount++;
				}
			}
			else {
				wasPreviousSep = false;
				wasThereAnyTokenBefore = true;
			}
			i++;
			ptrr += sizeof(char);
		}
		if(oryginalLen == 0) { oryginalLen = i; }
	}
	size_t oryginalSize = oryginalLen + 1;

	// TODO: Make it be only when the compiler has some debug flag
	printf("%zu, %zu, %zu, %zu", oryginalLen, oryginalSize, separatorCount, tokensCount);

	return (shl_tokens){0};
}

#endif
