Ultimate single alloc private:

typedef struct shl_tokens {
    const char** items;
	const size_t* count;
} shl_tokens;

lenght, count i capasity bierzesz przez funkcję.
lenght, count i capasity są na -, czyli:
[ count, lenght(0), capasity(0), lenght(1), capasity(1), ..., TU_WSKAZUJE_CHAR** -> "qwe", "ewq" ];
