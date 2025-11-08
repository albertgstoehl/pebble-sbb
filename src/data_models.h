#pragma once
#ifndef PEBBLE_H
#include <pebble.h>
#endif

#define MAX_STATION_NAME_LENGTH 32
#define MAX_STATION_ID_LENGTH 16
#define MAX_TRAIN_TYPE_LENGTH 16
#define MAX_PLATFORM_LENGTH 8
#define MAX_SAVED_CONNECTIONS 10
#define MAX_FAVORITE_STATIONS 20

// Station structure
typedef struct {
    char id[MAX_STATION_ID_LENGTH];
    char name[MAX_STATION_NAME_LENGTH];
    int distance_meters;
} Station;

// Favorite destination structure
typedef struct {
    char id[MAX_STATION_ID_LENGTH];
    char name[MAX_STATION_NAME_LENGTH];
    char label[16];  // "Home", "Work", "Gym", etc.
} FavoriteDestination;

#define MAX_FAVORITE_DESTINATIONS 10

// Saved connection structure
typedef struct {
    char departure_station_id[MAX_STATION_ID_LENGTH];
    char departure_station_name[MAX_STATION_NAME_LENGTH];
    char arrival_station_id[MAX_STATION_ID_LENGTH];
    char arrival_station_name[MAX_STATION_NAME_LENGTH];
} SavedConnection;

// Journey section (leg) structure
typedef struct {
    char departure_station[MAX_STATION_NAME_LENGTH];
    char arrival_station[MAX_STATION_NAME_LENGTH];
    time_t departure_time;
    time_t arrival_time;
    char platform[MAX_PLATFORM_LENGTH];
    char train_type[MAX_TRAIN_TYPE_LENGTH];
    int delay_minutes;
} JourneySection;

// Full connection structure (with sections)
typedef struct {
    JourneySection sections[5];  // Max 5 legs
    int num_sections;
    time_t departure_time;
    time_t arrival_time;
    int total_delay_minutes;
    int num_changes;
} Connection;

// Function declarations
SavedConnection create_saved_connection(
    const char *dep_id, const char *dep_name,
    const char *arr_id, const char *arr_name
);
Station create_station(const char *id, const char *name, int distance);
FavoriteDestination create_favorite_destination(
    const char *id, const char *name, const char *label
);
