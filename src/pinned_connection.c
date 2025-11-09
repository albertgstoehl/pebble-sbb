#include "pinned_connection.h"

#define PINNED_CONNECTION_KEY 100

void save_pinned_connection(PinnedConnection *pinned) {
    persist_write_data(PINNED_CONNECTION_KEY, pinned, sizeof(PinnedConnection));
    APP_LOG(APP_LOG_LEVEL_INFO, "Pinned connection saved, is_active=%d", pinned->is_active);
}

PinnedConnection load_pinned_connection(void) {
    PinnedConnection pinned;

    if (persist_exists(PINNED_CONNECTION_KEY)) {
        persist_read_data(PINNED_CONNECTION_KEY, &pinned, sizeof(PinnedConnection));
        APP_LOG(APP_LOG_LEVEL_INFO, "Loaded pinned connection, is_active=%d", pinned.is_active);
    } else {
        // Initialize empty
        memset(&pinned, 0, sizeof(PinnedConnection));
        pinned.is_active = false;
        APP_LOG(APP_LOG_LEVEL_INFO, "No pinned connection found, initialized empty");
    }

    return pinned;
}

void clear_pinned_connection(void) {
    PinnedConnection pinned;
    memset(&pinned, 0, sizeof(PinnedConnection));
    pinned.is_active = false;
    save_pinned_connection(&pinned);
    APP_LOG(APP_LOG_LEVEL_INFO, "Pinned connection cleared");
}

bool is_pinned_connection_expired(PinnedConnection *pinned) {
    if (!pinned->is_active) {
        return false;
    }

    time_t now = time(NULL);
    bool expired = pinned->connection.arrival_time < now;

    if (expired) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Pinned connection expired (arrival: %d, now: %d)",
                (int)pinned->connection.arrival_time, (int)now);
    }

    return expired;
}
