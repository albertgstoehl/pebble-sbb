#pragma once
#include "data_models.h"

#define PERSIST_KEY_CONNECTIONS 1
#define PERSIST_KEY_FAVORITES 2
#define PERSIST_KEY_NUM_CONNECTIONS 3
#define PERSIST_KEY_NUM_FAVORITES 4
#define PERSIST_KEY_FAVORITE_DESTINATIONS 5
#define PERSIST_KEY_NUM_FAVORITE_DESTINATIONS 6

// Save/load saved connections
void save_connections(SavedConnection *connections, int count);
int load_connections(SavedConnection *connections);

// Save/load favorite stations
void save_favorites(Station *stations, int count);
int load_favorites(Station *stations);

// Check if connection limit reached
bool is_connection_limit_reached(void);

// Save/load favorite destinations
void save_favorite_destinations(FavoriteDestination *favorites, int count);
int load_favorite_destinations(FavoriteDestination *favorites);
