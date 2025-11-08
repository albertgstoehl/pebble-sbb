#include "app_message.h"
#include "station_select_window.h"
#include "connection_detail_window.h"
#include "data_models.h"
#include "error_dialog.h"

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

    // Check for connection data
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
