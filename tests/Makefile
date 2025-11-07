CC = gcc
CFLAGS = -Wall -Wextra -g -I../src

# Mock pebble.h for testing
PEBBLE_MOCK = -include test_pebble_mock.h

test_data_models: test_data_models.c ../src/data_models.c
	$(CC) $(CFLAGS) $(PEBBLE_MOCK) -o $@ $^

clean:
	rm -f test_data_models test_persistence

.PHONY: clean
