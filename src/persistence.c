#include "persistence.h"

void save_connections(SavedConnection *connections, int count) {
    if (count > MAX_SAVED_CONNECTIONS) {
        count = MAX_SAVED_CONNECTIONS;
    }
    persist_write_int(PERSIST_KEY_NUM_CONNECTIONS, count);
    persist_write_data(PERSIST_KEY_CONNECTIONS, connections,
                       sizeof(SavedConnection) * count);
}

int load_connections(SavedConnection *connections) {
    if (!persist_exists(PERSIST_KEY_NUM_CONNECTIONS)) {
        return 0;
    }
    int count = persist_read_int(PERSIST_KEY_NUM_CONNECTIONS);
    persist_read_data(PERSIST_KEY_CONNECTIONS, connections,
                      sizeof(SavedConnection) * count);
    return count;
}

void save_favorites(Station *stations, int count) {
    if (count > MAX_FAVORITE_STATIONS) {
        count = MAX_FAVORITE_STATIONS;
    }
    persist_write_int(PERSIST_KEY_NUM_FAVORITES, count);
    persist_write_data(PERSIST_KEY_FAVORITES, stations,
                       sizeof(Station) * count);
}

int load_favorites(Station *stations) {
    if (!persist_exists(PERSIST_KEY_NUM_FAVORITES)) {
        return 0;
    }
    int count = persist_read_int(PERSIST_KEY_NUM_FAVORITES);
    persist_read_data(PERSIST_KEY_FAVORITES, stations,
                      sizeof(Station) * count);
    return count;
}

bool is_connection_limit_reached(void) {
    if (!persist_exists(PERSIST_KEY_NUM_CONNECTIONS)) {
        return false;
    }
    int count = persist_read_int(PERSIST_KEY_NUM_CONNECTIONS);
    return count >= MAX_SAVED_CONNECTIONS;
}
