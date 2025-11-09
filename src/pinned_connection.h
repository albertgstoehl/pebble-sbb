#pragma once
#include <pebble.h>
#include "data_models.h"

typedef struct {
    Connection connection;
    SavedConnection route;
    time_t pinned_at;
    bool is_active;
} PinnedConnection;

// Persistence
void save_pinned_connection(PinnedConnection *pinned);
PinnedConnection load_pinned_connection(void);
void clear_pinned_connection(void);

// Expiry check
bool is_pinned_connection_expired(PinnedConnection *pinned);
