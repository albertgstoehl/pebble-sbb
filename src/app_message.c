#include "app_message.h"
#include "station_select_window.h"
#include "connection_detail_window.h"
#include "data_models.h"
#include "error_dialog.h"
#include "persistence.h"

static FavoriteDestination s_temp_favorites[MAX_FAVORITE_DESTINATIONS];
static int s_temp_favorites_count = 0;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Check for station data
    Tuple *station_name_tuple = dict_find(iterator, MESSAGE_KEY_STATION_NAME);
    Tuple *station_id_tuple = dict_find(iterator, MESSAGE_KEY_STATION_ID);
    Tuple *station_distance_tuple = dict_find(iterator, MESSAGE_KEY_STATION_DISTANCE);

    if (station_name_tuple && station_id_tuple && station_distance_tuple) {
        Station station = create_station(
            station_id_tuple->value->cstring,
            station_name_tuple->value->cstring,
            station_distance_tuple->value->int32
        );
        station_select_window_add_station(station);
        return;
    }

    // Check for favorite destination data
    Tuple *fav_id_tuple = dict_find(iterator, MESSAGE_KEY_FAVORITE_DESTINATION_ID);
    Tuple *fav_name_tuple = dict_find(iterator, MESSAGE_KEY_FAVORITE_DESTINATION_NAME);
    Tuple *fav_label_tuple = dict_find(iterator, MESSAGE_KEY_FAVORITE_DESTINATION_LABEL);
    Tuple *num_favorites_tuple = dict_find(iterator, MESSAGE_KEY_NUM_FAVORITES);

    if (num_favorites_tuple) {
        // Start receiving new favorites list
        s_temp_favorites_count = 0;
        APP_LOG(APP_LOG_LEVEL_INFO, "Receiving %d favorites", num_favorites_tuple->value->int8);
        return;
    }

    if (fav_id_tuple && fav_name_tuple && fav_label_tuple) {
        // Receive individual favorite
        if (s_temp_favorites_count < MAX_FAVORITE_DESTINATIONS) {
            FavoriteDestination fav = create_favorite_destination(
                fav_id_tuple->value->cstring,
                fav_name_tuple->value->cstring,
                fav_label_tuple->value->cstring
            );
            s_temp_favorites[s_temp_favorites_count++] = fav;
            APP_LOG(APP_LOG_LEVEL_INFO, "Received favorite: %s - %s",
                    fav.label, fav.name);

            // If we've received all expected favorites, save them
            save_favorite_destinations(s_temp_favorites, s_temp_favorites_count);
        }
        return;
    }

    // Check for connection data
    // Note: Quick route feature reuses this same CONNECTION_DATA message type
    // No separate REQUEST_QUICK_ROUTE handler needed - same flow as regular connections
    Tuple *conn_data_tuple = dict_find(iterator, MESSAGE_KEY_CONNECTION_DATA);
    if (conn_data_tuple) {
        Connection conn;
        conn.num_sections = 1;  // Simple case for now

        Tuple *dep_time = dict_find(iterator, MESSAGE_KEY_DEPARTURE_TIME);
        Tuple *arr_time = dict_find(iterator, MESSAGE_KEY_ARRIVAL_TIME);
        Tuple *platform = dict_find(iterator, MESSAGE_KEY_PLATFORM);
        Tuple *train_type = dict_find(iterator, MESSAGE_KEY_TRAIN_TYPE);
        Tuple *delay = dict_find(iterator, MESSAGE_KEY_DELAY_MINUTES);
        Tuple *num_changes = dict_find(iterator, MESSAGE_KEY_NUM_CHANGES);

        if (dep_time && arr_time) {
            conn.departure_time = dep_time->value->int32;
            conn.arrival_time = arr_time->value->int32;
            conn.total_delay_minutes = delay ? delay->value->int8 : 0;
            conn.num_changes = num_changes ? num_changes->value->int8 : 0;

            // First section
            conn.sections[0].departure_time = conn.departure_time;
            conn.sections[0].arrival_time = conn.arrival_time;
            conn.sections[0].delay_minutes = conn.total_delay_minutes;

            if (platform) {
                strncpy(conn.sections[0].platform, platform->value->cstring, MAX_PLATFORM_LENGTH - 1);
            }
            if (train_type) {
                strncpy(conn.sections[0].train_type, train_type->value->cstring, MAX_TRAIN_TYPE_LENGTH - 1);
            }

            connection_detail_window_update_data(&conn, 1);
        }
        return;
    }

    // Check for error
    Tuple *error_tuple = dict_find(iterator, MESSAGE_KEY_ERROR_MESSAGE);
    if (error_tuple) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error from JS: %s", error_tuple->value->cstring);
        show_error_dialog("Error", error_tuple->value->cstring);
        return;
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox sent successfully");
}

void app_message_init(void) {
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    app_message_open(512, 512);
}

void app_message_deinit(void) {
    app_message_deregister_callbacks();
}
