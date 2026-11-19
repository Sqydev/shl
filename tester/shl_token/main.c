#include "shl_token.h"

int main(void) {
	shl_token_opts def = shl_get_default_opts();
	shl_tokens tokens = shl_tokenise("  Hello       world   \n     ", &def);
	(void)tokens;

	return 0;
}
