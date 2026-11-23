#include "shl_token.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHL_TEST_CONFIG_COUNT 8
#define SHL_TEST_COUNT 8

typedef struct shl_test_stats {
	size_t passed;
	size_t failed;
} shl_test_stats;

typedef struct shl_test_case {
	const char* name;
	const char* input;
	const char** expected;
	size_t expectedCount;
} shl_test_case;

typedef struct shl_failed_test {
	size_t number;
	char id[16];
	char config[64];
	char test[64];
	char reason[128];
} shl_failed_test;

static shl_failed_test failures[SHL_TEST_CONFIG_COUNT * SHL_TEST_COUNT];
static size_t failureCount = 0;
static shl_test_stats globalStats = { 0, 0 };

static void shl_test_record_failure(const char* id, const char* configName, const shl_test_case* testCase, const char* reason) {
	if(failureCount >= SHL_TEST_CONFIG_COUNT * SHL_TEST_COUNT) { return; }
	shl_failed_test* failure = &failures[failureCount];

	failure->number = failureCount + 1;
	snprintf(failure->id, sizeof(failure->id), "%s", id);
	snprintf(failure->config, sizeof(failure->config), "%s", configName);
	snprintf(failure->test, sizeof(failure->test), "%s", testCase->name);
	snprintf(failure->reason, sizeof(failure->reason), "%s", reason);

	failureCount++;
}

static void shl_test(shl_test_stats* stats, const char* id, const char* configName, const shl_test_case* testCase, shl_token_opts opts) {
	shl_tokens result = shl_tokenise(testCase->input, &opts);
	char reason[128] = "";
	size_t count = 0;

	if(!result.items || !result.count) {
		snprintf(reason, sizeof(reason), "%s", "tokenise returned NULL");
	}
	else {
		count = result.count[0];

		if(count != testCase->expectedCount) { snprintf(reason, sizeof(reason), "count: expected %zu, got %zu", testCase->expectedCount, count); }

		for(size_t i = 0; i < testCase->expectedCount && i < count; i++) {
			if(strcmp(result.items[i], testCase->expected[i]) != 0) {
				if(reason[0] == '\0') { snprintf(reason, sizeof(reason), "token[%zu]: expected \"%s\", got \"%s\"", i, testCase->expected[i], result.items[i]); }
				break;
			}
		}

		free((void*)result.count);
	}

	if(reason[0] == '\0') {
		printf("[PASS] %-10s %-24s | %s\n", id, configName, testCase->name);
		stats->passed++;
		globalStats.passed++;
		return;
	}

	stats->failed++;
	globalStats.failed++;

	printf("[FAIL #%zu] %-10s %-24s | %s: %s\n", failureCount + 1, id, configName, testCase->name, reason);
	printf("             input: \"%s\"\n", testCase->input);

	shl_test_record_failure(id, configName, testCase, reason);
}

static void shl_test_run_suite(size_t configIndex, const char* configName, shl_token_opts opts) {
	shl_test_stats stats = { 0, 0 };

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

	shl_test_case cases[] = {
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

	printf("\n");
	printf("===============================================================\n");
	printf(" CONFIG %zu/%zu: %s\n", configIndex, (size_t)SHL_TEST_CONFIG_COUNT, configName);
	printf("===============================================================\n");

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		char id[16];

		snprintf(id, sizeof(id), "C%zu/T%02zu", configIndex, i + 1);
		shl_test(&stats, id, configName, &cases[i], opts);
	}

	printf("---------------------------------------------------------------\n");
	printf("Passed: %zu | Failed: %zu | Total: %zu\n", stats.passed, stats.failed, stats.passed + stats.failed);
}

int main(void) {
	size_t configIndex = 0;

	for(int quotes = 0; quotes <= 1; quotes++) {
		for(int escape = 0; escape <= 1; escape++) {
			for(int keepQuotes = 0; keepQuotes <= 1; keepQuotes++) {
				shl_token_opts opts = shl_get_default_opts();
				char configName[64];

				opts.quotes = quotes;
				opts.quouteEscape = escape;
				opts.keepQuotes = keepQuotes;

				configIndex++;
				snprintf(configName, sizeof(configName), "quotes=%d escape=%d keep=%d", quotes, escape, keepQuotes);

				shl_test_run_suite(configIndex, configName, opts);
			}
		}
	}

	printf("\n\n");
	printf("===============================================================\n");
	printf("                       GLOBAL SUMMARY\n");
	printf("===============================================================\n");
	printf("Total:  %zu\n", globalStats.passed + globalStats.failed);
	printf("Passed: %zu\n", globalStats.passed);
	printf("Failed: %zu\n", globalStats.failed);
	printf("\n");

	if(failureCount == 0) {
		printf("[SUCCESS] All tests passed!\n");
	}
	else {
		printf("FAILED TESTS (%zu):\n", failureCount);

		for(size_t i = 0; i < failureCount; i++) {
			printf("  #%zu  %-10s %-24s %-24s %s\n", failures[i].number, failures[i].id, failures[i].config, failures[i].test, failures[i].reason);
		}
	}

	printf("\n");

	return globalStats.failed != 0;
}
