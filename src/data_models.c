#include "data_models.h"
#include <string.h>

SavedConnection create_saved_connection(
    const char *dep_id, const char *dep_name,
    const char *arr_id, const char *arr_name
) {
    SavedConnection conn;
    memset(&conn, 0, sizeof(SavedConnection));

    // Validate NULL pointers
    if (!dep_id || !dep_name || !arr_id || !arr_name) {
        return conn;
    }

    strncpy(conn.departure_station_id, dep_id, MAX_STATION_ID_LENGTH - 1);
    conn.departure_station_id[MAX_STATION_ID_LENGTH - 1] = '\0';
    strncpy(conn.departure_station_name, dep_name, MAX_STATION_NAME_LENGTH - 1);
    conn.departure_station_name[MAX_STATION_NAME_LENGTH - 1] = '\0';
    strncpy(conn.arrival_station_id, arr_id, MAX_STATION_ID_LENGTH - 1);
    conn.arrival_station_id[MAX_STATION_ID_LENGTH - 1] = '\0';
    strncpy(conn.arrival_station_name, arr_name, MAX_STATION_NAME_LENGTH - 1);
    conn.arrival_station_name[MAX_STATION_NAME_LENGTH - 1] = '\0';
    return conn;
}

Station create_station(const char *id, const char *name, int distance) {
    Station station;
    memset(&station, 0, sizeof(Station));

    // Validate NULL pointers
    if (!id || !name) {
        return station;
    }

    strncpy(station.id, id, MAX_STATION_ID_LENGTH - 1);
    station.id[MAX_STATION_ID_LENGTH - 1] = '\0';
    strncpy(station.name, name, MAX_STATION_NAME_LENGTH - 1);
    station.name[MAX_STATION_NAME_LENGTH - 1] = '\0';
    station.distance_meters = distance;
    return station;
}

FavoriteDestination create_favorite_destination(
    const char *id, const char *name, const char *label
) {
    FavoriteDestination favorite;
    memset(&favorite, 0, sizeof(FavoriteDestination));

    // Validate NULL pointers
    if (!id || !name || !label) {
        return favorite;
    }

    strncpy(favorite.id, id, MAX_STATION_ID_LENGTH - 1);
    favorite.id[MAX_STATION_ID_LENGTH - 1] = '\0';

    strncpy(favorite.name, name, MAX_STATION_NAME_LENGTH - 1);
    favorite.name[MAX_STATION_NAME_LENGTH - 1] = '\0';

    strncpy(favorite.label, label, 15);
    favorite.label[15] = '\0';

    return favorite;
}
