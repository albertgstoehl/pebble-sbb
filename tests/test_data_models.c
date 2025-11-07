#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/data_models.h"

void test_create_saved_connection(void) {
    SavedConnection conn = create_saved_connection(
        "8503000", "Zürich HB",
        "8507000", "Bern"
    );

    assert(strcmp(conn.departure_station_id, "8503000") == 0);
    assert(strcmp(conn.departure_station_name, "Zürich HB") == 0);
    assert(strcmp(conn.arrival_station_id, "8507000") == 0);
    assert(strcmp(conn.arrival_station_name, "Bern") == 0);

    printf("test_create_saved_connection: PASS\n");
}

void test_create_station(void) {
    Station station = create_station("8503000", "Zürich HB", 1200);

    assert(strcmp(station.id, "8503000") == 0);
    assert(strcmp(station.name, "Zürich HB") == 0);
    assert(station.distance_meters == 1200);

    printf("test_create_station: PASS\n");
}

void test_truncate_long_station_name(void) {
    char long_name[100];
    memset(long_name, 'A', 99);
    long_name[99] = '\0';

    Station station = create_station("123", long_name, 100);

    // Should truncate to MAX_STATION_NAME_LENGTH - 1
    assert(strlen(station.name) < 100);
    assert(station.name[MAX_STATION_NAME_LENGTH - 1] == '\0');

    printf("test_truncate_long_station_name: PASS\n");
}

int main(void) {
    test_create_saved_connection();
    test_create_station();
    test_truncate_long_station_name();
    printf("\nAll data_models tests passed!\n");
    return 0;
}
