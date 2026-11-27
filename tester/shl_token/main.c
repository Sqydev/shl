#define _POSIX_C_SOURCE 200809L

#ifdef SHL_TEST_DEBUG
	#define SHL_TOKEN_DEBUG 1
#else
	#undef SHL_TOKEN_DEBUG
#endif

#include "shl_token.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SHL_MAX_RESULTS 256
#define SHL_REASON_SIZE 192
#define SHL_LINE_WIDTH 64

#define SHL_C_RESET "\033[0m"
#define SHL_C_BOLD "\033[1m"
#define SHL_C_DIM "\033[2m"
#define SHL_C_GREEN "\033[32m"
#define SHL_C_RED "\033[31m"
#define SHL_C_CYAN "\033[36m"

typedef struct shl_check {
	char section[128];
	char id[16];
	const char* name;
	bool passed;
	char reason[SHL_REASON_SIZE];
	const char* input;
} shl_check;

typedef struct shl_token_case {
	const char* name;
	const char* input;
	const char* const* expected;
	size_t expectedCount;
} shl_token_case;

typedef struct shl_config {
	const char* name;
	bool quotes;
	bool escape;
	bool keepQuotes;
} shl_config;

typedef void (*shl_check_fn)(shl_check*);

typedef struct shl_fn_test {
	const char* id;
	const char* name;
	shl_check_fn fn;
} shl_fn_test;

static shl_check results[SHL_MAX_RESULTS];
static size_t resultCount = 0;
static size_t passedCount = 0;
static bool useColor = false;

static const shl_config configs[] = {
	{ "quotes=0 escape=0 keep=0", false, false, false },
	{ "quotes=0 escape=0 keep=1", false, false, true },
	{ "quotes=0 escape=1 keep=0", false, true, false },
	{ "quotes=0 escape=1 keep=1", false, true, true },
	{ "quotes=1 escape=0 keep=0", true, false, false },
	{ "quotes=1 escape=0 keep=1", true, false, true },
	{ "quotes=1 escape=1 keep=0", true, true, false },
	{ "quotes=1 escape=1 keep=1", true, true, true },
};

static const char* color(const char* code) {
	return useColor ? code : "";
}

static void shl_fail(shl_check* check, const char* fmt, ...) {
	if(!check->passed) { return; }
	check->passed = false;

	va_list args;
	va_start(args, fmt);
	vsnprintf(check->reason, sizeof(check->reason), fmt, args);
	va_end(args);
}

static void shl_expect_size(shl_check* check, const char* what, size_t expected, size_t actual) {
	if(expected != actual) { shl_fail(check, "%s: expected %zu, got %zu", what, expected, actual); }
}

static void shl_expect_char(shl_check* check, const char* what, char expected, char actual) {
	if(expected != actual) { shl_fail(check, "%s: expected '%c', got '%c'", what, expected, actual); }
}

static void shl_expect_bool(shl_check* check, const char* what, bool expected, bool actual) {
	if(expected != actual) { shl_fail(check, "%s: expected %s, got %s", what, expected ? "true" : "false", actual ? "true" : "false"); }
}

static void shl_record(shl_check* check) {
	if(resultCount >= SHL_MAX_RESULTS) { return; }
	results[resultCount++] = *check;
	if(check->passed) { passedCount++; }
}

static void shl_run_check(const char* section, const char* id, const char* name, shl_check_fn fn) {
	shl_check check = { { 0 }, { 0 }, name, true, { 0 }, NULL };
	snprintf(check.section, sizeof(check.section), "%s", section);
	snprintf(check.id, sizeof(check.id), "%s", id);
	fn(&check);
	shl_record(&check);
}

static void shl_check_tokens(shl_check* check, const char* input, shl_token_opts opts, const char* const* expected, size_t expectedCount) {
	check->input = input;

	shl_tokens tokens = shl_token_tokenise(input, &opts);
	if(!tokens.items || !tokens.count) {
		shl_fail(check, "tokenise returned NULL");
		return;
	}

	size_t count = tokens.count[0];
	shl_expect_size(check, "count", expectedCount, count);

	size_t common = expectedCount < count ? expectedCount : count;
	for(size_t i = 0; i < common; i++) {
		size_t expectedLen = strlen(expected[i]);
		size_t actualLen = shl_token_toklen(tokens, i);

		shl_expect_size(check, "token length", expectedLen, actualLen);
		if(actualLen != expectedLen) { break; }

		if(memcmp(tokens.items[i], expected[i], actualLen) != 0) {
			shl_fail(check, "token[%zu]: expected \"%.*s\", got \"%.*s\"", i, (int)expectedLen, expected[i], (int)actualLen, tokens.items[i]);
			break;
		}

		if(tokens.items[i][actualLen] != opts.nullTerminatorOut) {
			shl_fail(check, "token[%zu]: terminator expected '%c', got '%c'", i, opts.nullTerminatorOut, tokens.items[i][actualLen]);
			break;
		}
	}

	shl_token_free(tokens);
}

static void shl_run_config(size_t index, shl_config config) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.quotes = config.quotes;
	opts.quouteEscape = config.escape;
	opts.keepQuotes = config.keepQuotes;

	const char* basic[] = { "Hello", "world" };
	const char* multipleSeparators[] = { "Hello", "world" };
	const char* quotedKeep[] = { "\"very wholesome token\"" };
	const char* quotedRemove[] = { "very wholesome token" };
	const char* quotedDisabled[] = { "\"very", "wholesome", "token\"" };
	const char* quotedSpacesKeep[] = { "Hello", "\"very wholesome token\"", "world" };
	const char* quotedSpacesRemove[] = { "Hello", "very wholesome token", "world" };
	const char* quotedSpacesDisabled[] = { "Hello", "\"very", "wholesome", "token\"", "world" };
	const char* enterSpaces[] = { "Hello", "world", "\n" };
	const char* enterSpacesNS[] = { "Hello", "world\n" };
	const char* escapedKeep[] = { "\"a\"b\"" };
	const char* escapedRemove[] = { "a\"b" };
	const char* escapedRawKeep[] = { "\"a\\\"b\"" };
	const char* escapedRawRemove[] = { "a\\b" };
	const char* empty[] = { NULL };

	shl_token_case cases[] = {
		{ "Basic", "Hello world", basic, 2 },
		{ "Multiple separators", "  Hello       world   ", multipleSeparators, 2 },
		{
			"Quoted",
			"\"very wholesome token\"",
			opts.quotes ? (opts.keepQuotes ? quotedKeep : quotedRemove) : quotedDisabled,
			opts.quotes ? 1 : 3
		},
		{
			"Quoted with spaces",
			"Hello \"very wholesome token\" world",
			opts.quotes ? (opts.keepQuotes ? quotedSpacesKeep : quotedSpacesRemove) : quotedSpacesDisabled,
			opts.quotes ? 3 : 5
		},
		{ "Empty input", "", empty, 0 },
		{ "Enter spaces", "Hello world \n", enterSpaces, 3 },
		{ "Enter spaces no sep", "Hello world\n", enterSpacesNS, 2 },
		{
			"Escaped quote",
			"\"a\\\"b\"",
			opts.quotes
				? (opts.quouteEscape
					? (opts.keepQuotes ? escapedKeep : escapedRemove)
					: (opts.keepQuotes ? escapedRawKeep : escapedRawRemove))
				: (opts.quouteEscape ? escapedKeep : escapedRawKeep),
			1
		}
	};

	char section[128];
	snprintf(section, sizeof(section), "CONFIG %zu/%zu - %s", index, sizeof(configs) / sizeof(configs[0]), config.name);

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		char id[16];
		snprintf(id, sizeof(id), "C%zu/T%02zu", index, i + 1);

		shl_check check = { { 0 }, { 0 }, cases[i].name, true, { 0 }, NULL };
		snprintf(check.section, sizeof(check.section), "%s", section);
		snprintf(check.id, sizeof(check.id), "%s", id);
		shl_check_tokens(&check, cases[i].input, opts, cases[i].expected, cases[i].expectedCount);
		shl_record(&check);
	}
}

static void shl_test_default_opts(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();

	shl_expect_size(check, "size", 0, opts.size);
	shl_expect_char(check, "nullTerminatorIn", '\0', opts.nullTerminatorIn);
	shl_expect_char(check, "nullTerminatorOut", '\0', opts.nullTerminatorOut);
	shl_expect_char(check, "separator", ' ', opts.separator);
	shl_expect_bool(check, "quotes", true, opts.quotes);
	shl_expect_char(check, "quetesChar", '"', opts.quetesChar);
	shl_expect_bool(check, "quouteEscape", true, opts.quouteEscape);
	shl_expect_char(check, "quouteEscapeChar", '\\', opts.quouteEscapeChar);
	shl_expect_bool(check, "keepQuotes", true, opts.keepQuotes);
}

static void shl_test_toklen(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.keepQuotes = false;

	shl_tokens tokens = shl_token_tokenise("a \"bb cc\" d", &opts);
	if(!tokens.items || !tokens.count) {
		shl_fail(check, "tokenise returned NULL");
		return;
	}

	size_t expectedLengths[] = { 1, 5, 1 };
	shl_expect_size(check, "count", 3, tokens.count[0]);
	for(size_t i = 0; i < sizeof(expectedLengths) / sizeof(expectedLengths[0]) && i < tokens.count[0]; i++) {
		shl_expect_size(check, "toklen", expectedLengths[i], shl_token_toklen(tokens, i));
	}

	shl_token_free(tokens);
}

static void shl_test_toklen_keep_quotes(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();

	shl_tokens tokens = shl_token_tokenise("a \"bb cc\" d", &opts);
	if(!tokens.items || !tokens.count) {
		shl_fail(check, "tokenise returned NULL");
		return;
	}

	size_t expectedLengths[] = { 1, 7, 1 };
	shl_expect_size(check, "count", 3, tokens.count[0]);
	for(size_t i = 0; i < sizeof(expectedLengths) / sizeof(expectedLengths[0]) && i < tokens.count[0]; i++) {
		shl_expect_size(check, "toklen", expectedLengths[i], shl_token_toklen(tokens, i));
	}

	shl_token_free(tokens);
}

static void shl_test_toklen_escape(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.keepQuotes = false;

	shl_tokens tokens = shl_token_tokenise("\"a\\\"b\"", &opts);
	if(!tokens.items || !tokens.count) {
		shl_fail(check, "tokenise returned NULL");
		return;
	}

	shl_expect_size(check, "count", 1, tokens.count[0]);
	shl_expect_size(check, "toklen", 3, shl_token_toklen(tokens, 0));
	if(memcmp(tokens.items[0], "a\"b", 3) != 0) { shl_fail(check, "token[0]: expected \"a\\\"b\", got \"%.*s\"", 3, tokens.items[0]); }

	shl_token_free(tokens);
}

static void shl_test_toklen_custom_terminator(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.nullTerminatorOut = '|';

	const char* expected[] = { "a", "bb", "c" };
	shl_check_tokens(check, "a bb c", opts, expected, 3);
}

static void shl_test_toklen_out_of_range(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();

	shl_tokens tokens = shl_token_tokenise("a b", &opts);
	if(!tokens.items || !tokens.count) {
		shl_fail(check, "tokenise returned NULL");
		return;
	}

	shl_expect_size(check, "toklen(count)", 0, shl_token_toklen(tokens, tokens.count[0]));
	shl_expect_size(check, "toklen(count+8)", 0, shl_token_toklen(tokens, tokens.count[0] + 8));

	shl_token_free(tokens);
}

static void shl_test_null_input(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();

	shl_tokens nullInput = shl_token_tokenise(NULL, &opts);
	if(nullInput.items || nullInput.count) { shl_fail(check, "NULL input should return empty tokens"); }
	shl_token_free(nullInput);

	shl_tokens nullOpts = shl_token_tokenise("a", NULL);
	if(nullOpts.items || nullOpts.count) { shl_fail(check, "NULL opts should return empty tokens"); }
	shl_token_free(nullOpts);
}

static void shl_test_explicit_size(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.size = 5;

	const char* expected[] = { "Hello" };
	shl_check_tokens(check, "Hello world", opts, expected, 1);
}

static void shl_test_escape_at_token_start(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	const char* expected[] = { "\"" };

	shl_check_tokens(check, "\\\"", opts, expected, 1);

	opts.keepQuotes = false;
	shl_check_tokens(check, "\\\"", opts, expected, 1);
}

static void shl_test_escape_after_separator(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	const char* expected[] = { "a", "\"" };

	shl_check_tokens(check, "a \\\"", opts, expected, 2);
}

static void shl_test_escape_of_separator(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.quotes = false;
	opts.separator = '"';
	const char* expected[] = { "a", "b" };

	shl_check_tokens(check, "a\\\"b", opts, expected, 2);
}

static void shl_test_ftokenise(shl_check* check) {
	FILE* file = tmpfile();
	if(!file) { shl_fail(check, "tmpfile failed"); return; }

	fputs("Hello world", file);
	rewind(file);

	shl_token_opts opts = shl_token_get_defaults();
	shl_tokens tokens = shl_token_ftokenise(file, &opts);
	fclose(file);

	if(!tokens.items || !tokens.count) {
		shl_fail(check, "ftokenise returned NULL");
		return;
	}

	shl_expect_size(check, "count", 2, tokens.count[0]);
	shl_expect_size(check, "token[0] length", 5, shl_token_toklen(tokens, 0));
	shl_expect_size(check, "token[1] length", 5, shl_token_toklen(tokens, 1));
	if(memcmp(tokens.items[0], "Hello", 5) != 0) { shl_fail(check, "token[0] mismatch"); }
	if(memcmp(tokens.items[1], "world", 5) != 0) { shl_fail(check, "token[1] mismatch"); }

	shl_token_free(tokens);
}

static void shl_test_ftokenise_size_limit(shl_check* check) {
	FILE* file = tmpfile();
	if(!file) { shl_fail(check, "tmpfile failed"); return; }

	fputs("Hello world", file);
	rewind(file);

	shl_token_opts opts = shl_token_get_defaults();
	opts.size = 5;
	shl_tokens tokens = shl_token_ftokenise(file, &opts);
	fclose(file);

	if(!tokens.items || !tokens.count) {
		shl_fail(check, "ftokenise returned NULL");
		return;
	}

	shl_expect_size(check, "count", 1, tokens.count[0]);
	shl_expect_size(check, "toklen", 5, shl_token_toklen(tokens, 0));
	if(memcmp(tokens.items[0], "Hello", 5) != 0) { shl_fail(check, "token[0] mismatch"); }

	shl_token_free(tokens);
}

static void shl_test_ftokenise_terminator(shl_check* check) {
	FILE* file = tmpfile();
	if(!file) { shl_fail(check, "tmpfile failed"); return; }

	fputs("a|b c", file);
	rewind(file);

	shl_token_opts opts = shl_token_get_defaults();
	opts.nullTerminatorIn = '|';
	shl_tokens tokens = shl_token_ftokenise(file, &opts);
	fclose(file);

	if(!tokens.items || !tokens.count) {
		shl_fail(check, "ftokenise returned NULL");
		return;
	}

	shl_expect_size(check, "count", 1, tokens.count[0]);
	shl_expect_size(check, "toklen", 1, shl_token_toklen(tokens, 0));
	if(memcmp(tokens.items[0], "a", 1) != 0) { shl_fail(check, "token[0] mismatch"); }

	shl_token_free(tokens);
}

static void shl_test_escape_equals_quote(shl_check* check) {
	shl_token_opts opts = shl_token_get_defaults();
	opts.quetesChar = '"';
	opts.quouteEscapeChar = '"';

	const char* withQuotes[] = { "\"x\" y" };
	shl_check_tokens(check, "\"x\"\" y", opts, withQuotes, 1);

	opts.keepQuotes = false;
	const char* withoutQuotes[] = { "x\" y" };
	shl_check_tokens(check, "\"x\"\" y", opts, withoutQuotes, 1);

	const char* escapedStart[] = { "\"", "x" };
	shl_check_tokens(check, "\"\" x", opts, escapedStart, 2);
}

static void shl_test_const_opts(shl_check* check) {
	const shl_token_opts opts = shl_token_get_defaults();
	shl_tokens tokens = shl_token_tokenise("a b", &opts);
	if(!tokens.items || !tokens.count) {
		shl_fail(check, "tokenise returned NULL");
		return;
	}

	shl_expect_size(check, "count", 2, tokens.count[0]);
	shl_token_free(tokens);
}

static void shl_test_ftokenise_embedded_nul(shl_check* check) {
	FILE* file = tmpfile();
	if(!file) { shl_fail(check, "tmpfile failed"); return; }

	const char contents[] = { 'a', '\0', 'b', ' ', 'c' };
	if(fwrite(contents, 1, sizeof(contents), file) != sizeof(contents)) {
		fclose(file);
		shl_fail(check, "fwrite failed");
		return;
	}
	rewind(file);

	shl_token_opts opts = shl_token_get_defaults();
	shl_tokens tokens = shl_token_ftokenise(file, &opts);
	fclose(file);

	if(!tokens.items || !tokens.count) {
		shl_fail(check, "ftokenise returned NULL");
		return;
	}

	shl_expect_size(check, "count", 1, tokens.count[0]);
	shl_expect_size(check, "toklen", 1, shl_token_toklen(tokens, 0));
	if(memcmp(tokens.items[0], "a", 1) != 0) { shl_fail(check, "token[0] mismatch"); }

	shl_token_free(tokens);
}

static const shl_fn_test fnTests[] = {
	{ "F01", "Default options", shl_test_default_opts },
	{ "F02", "toklen basic", shl_test_toklen },
	{ "F03", "toklen keep quotes", shl_test_toklen_keep_quotes },
	{ "F04", "toklen escaped quote", shl_test_toklen_escape },
	{ "F05", "toklen custom terminator", shl_test_toklen_custom_terminator },
	{ "F06", "toklen out of range", shl_test_toklen_out_of_range },
	{ "F07", "NULL input and opts", shl_test_null_input },
	{ "F08", "Explicit size", shl_test_explicit_size },
	{ "F09", "Escape at token start", shl_test_escape_at_token_start },
	{ "F10", "Escape after separator", shl_test_escape_after_separator },
	{ "F11", "Escape of separator", shl_test_escape_of_separator },
	{ "F12", "ftokenise basic", shl_test_ftokenise },
	{ "F13", "ftokenise size limit", shl_test_ftokenise_size_limit },
	{ "F14", "ftokenise terminator", shl_test_ftokenise_terminator },
	{ "F15", "escape char equals quote char", shl_test_escape_equals_quote },
	{ "F16", "const opts", shl_test_const_opts },
	{ "F17", "ftokenise embedded NUL", shl_test_ftokenise_embedded_nul },
};

static void shl_separator(const char* title) {
	size_t titleLen = strlen(title);

	printf("\n%s", color(SHL_C_DIM));
	printf("-- %s ", title);
	for(size_t i = titleLen + 4; i < SHL_LINE_WIDTH; i++) { printf("-"); }
	printf("%s\n", color(SHL_C_RESET));
}

static void shl_print_results(void) {
	const char* section = NULL;
	size_t failedNumber = 0;

	for(size_t i = 0; i < resultCount; i++) {
		const shl_check* result = &results[i];

		if(!section || strcmp(section, result->section) != 0) {
			section = result->section;
			shl_separator(section);
		}

		const char* symbol = result->passed ? "+" : "!";
		const char* symbolColor = result->passed ? SHL_C_GREEN : SHL_C_RED;
		printf("  %s%s%s %s%-9s%s %s%-34s%s", color(symbolColor), symbol, color(SHL_C_RESET), color(SHL_C_DIM), result->id, color(SHL_C_RESET), color(SHL_C_BOLD), result->name, color(SHL_C_RESET));

		if(!result->passed) {
			failedNumber++;
			printf("  %s%s%s", color(SHL_C_RED), result->reason, color(SHL_C_RESET));
		}
		printf("\n");

		if(!result->passed && result->input) { printf("      %sinput: \"%s\"%s\n", color(SHL_C_DIM), result->input, color(SHL_C_RESET)); }
	}

	size_t failed = resultCount - passedCount;
	shl_separator("SUMMARY");
	printf("  total:  %zu\n", resultCount);
	printf("  %spassed: %zu%s\n", color(SHL_C_GREEN), passedCount, color(SHL_C_RESET));
	printf("  %sfailed: %zu%s\n", failed ? color(SHL_C_RED) : color(SHL_C_DIM), failed, color(SHL_C_RESET));

	if(failed == 0) {
		printf("\n  %sAll tests passed.%s\n", color(SHL_C_GREEN), color(SHL_C_RESET));
	}
	else {
		printf("\n  %sFailed tests:%s\n", color(SHL_C_RED), color(SHL_C_RESET));

		failedNumber = 0;
		for(size_t i = 0; i < resultCount; i++) {
			if(results[i].passed) { continue; }
			failedNumber++;
			printf("    #%zu %-9s %-30s %s\n", failedNumber, results[i].id, results[i].name, results[i].reason);
		}
	}
}

int main(void) {
	useColor = isatty(fileno(stdout)) != 0;

	shl_separator("SHL TOKEN TESTER");
	printf("  %ssingle-header tokenizer test suite%s\n", color(SHL_C_BOLD), color(SHL_C_RESET));
	printf("  configs: %zu | token cases per config: 8 | function checks: %zu\n", sizeof(configs) / sizeof(configs[0]), sizeof(fnTests) / sizeof(fnTests[0]));

	for(size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
		shl_run_config(i + 1, configs[i]);
	}

	for(size_t i = 0; i < sizeof(fnTests) / sizeof(fnTests[0]); i++) {
		shl_run_check("FUNCTIONS", fnTests[i].id, fnTests[i].name, fnTests[i].fn);
	}

	shl_print_results();
	printf("\n");

	return resultCount != passedCount;
}
