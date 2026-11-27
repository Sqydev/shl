#define SHL_TOKEN_IMPLEMENTATION
#include "shl_token.h"

#include <cstring>

int main() {
	shl_token_opts opts = shl_token_get_defaults();
	shl_tokens tokens = shl_token_tokenise("a \"bb cc\" d", &opts);
	if(!tokens.items || !tokens.count) { return 1; }
	if(tokens.count[0] != 3) { return 1; }
	if(shl_token_toklen(tokens, 0) != 1) { return 1; }
	if(shl_token_toklen(tokens, 1) != 7) { return 1; }
	if(shl_token_toklen(tokens, 2) != 1) { return 1; }
	if(std::memcmp(tokens.items[1], "\"bb cc\"", 7) != 0) { return 1; }
	shl_token_free(tokens);
	return 0;
}
