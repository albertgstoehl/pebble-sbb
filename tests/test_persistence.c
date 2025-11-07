#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/persistence.h"

// Mock persist functions for testing
#define PERSIST_DATA_MAX_LENGTH 4096
static uint8_t persist_storage[10][PERSIST_DATA_MAX_LENGTH];
static bool persist_exists_flags[10];

bool persist_exists(uint32_t key) {
    return persist_exists_flags[key];
}

int persist_write_data(uint32_t key, const void *data, size_t size) {
    memcpy(persist_storage[key], data, size);
    persist_exists_flags[key] = true;
    return size;
}

int persist_read_data(uint32_t key, void *buffer, size_t size) {
    memcpy(buffer, persist_storage[key], size);
    return size;
}

int persist_write_int(uint32_t key, int value) {
    memcpy(persist_storage[key], &value, sizeof(int));
    persist_exists_flags[key] = true;
    return sizeof(int);
}

int persist_read_int(uint32_t key) {
    int value;
    memcpy(&value, persist_storage[key], sizeof(int));
    return value;
}

void test_save_and_load_connections(void) {
    SavedConnection conns[2];
    conns[0] = create_saved_connection("8503000", "Zürich HB", "8507000", "Bern");
    conns[1] = create_saved_connection("8507000", "Bern", "8508500", "Interlaken");

    save_connections(conns, 2);

    SavedConnection loaded[MAX_SAVED_CONNECTIONS];
    int count = load_connections(loaded);

    assert(count == 2);
    assert(strcmp(loaded[0].departure_station_name, "Zürich HB") == 0);
    assert(strcmp(loaded[1].arrival_station_name, "Interlaken") == 0);

    printf("test_save_and_load_connections: PASS\n");
}

void test_load_when_empty(void) {
    // Clear storage
    memset(persist_exists_flags, 0, sizeof(persist_exists_flags));

    SavedConnection loaded[MAX_SAVED_CONNECTIONS];
    int count = load_connections(loaded);

    assert(count == 0);

    printf("test_load_when_empty: PASS\n");
}

void test_connection_limit_reached(void) {
    SavedConnection conns[MAX_SAVED_CONNECTIONS];
    for (int i = 0; i < MAX_SAVED_CONNECTIONS; i++) {
        conns[i] = create_saved_connection("123", "A", "456", "B");
    }
    save_connections(conns, MAX_SAVED_CONNECTIONS);

    assert(is_connection_limit_reached() == true);

    printf("test_connection_limit_reached: PASS\n");
}

int main(void) {
    test_save_and_load_connections();
    test_load_when_empty();
    test_connection_limit_reached();
    printf("\nAll persistence tests passed!\n");
    return 0;
}
