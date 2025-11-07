#ifndef TEST_PEBBLE_MOCK_H
#define TEST_PEBBLE_MOCK_H

// Mock Pebble SDK types for testing
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stddef.h>

// Prevent actual pebble.h from being included
#define PEBBLE_H

// Mock persist function declarations
bool persist_exists(uint32_t key);
int persist_write_data(uint32_t key, const void *data, size_t size);
int persist_read_data(uint32_t key, void *buffer, size_t size);
int persist_write_int(uint32_t key, int value);
int persist_read_int(uint32_t key);

#endif
