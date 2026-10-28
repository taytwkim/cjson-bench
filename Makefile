CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic

benchmark: main.c cjson/cJSON.c cjson/cJSON.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 -pthread main.c cjson/cJSON.c $(LDFLAGS) $(LDLIBS) -lm -o $@

.PHONY: clean

clean:
	rm -f benchmark
