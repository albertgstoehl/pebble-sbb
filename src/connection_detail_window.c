#include "connection_detail_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static SavedConnection s_connection;
static Connection s_connections[5];
static int s_num_connections = 0;
static TextLayer *s_status_layer;
static AppTimer *s_refresh_timer;

#define REFRESH_INTERVAL_MS 60000  // 60 seconds

static void request_connections(void);

static void format_time(time_t timestamp, char *buffer, size_t size) {
    struct tm *tm_info = localtime(&timestamp);
    if (tm_info) {
        strftime(buffer, size, "%H:%M", tm_info);
    } else {
        snprintf(buffer, size, "??:??");
        APP_LOG(APP_LOG_LEVEL_ERROR, "localtime returned NULL for timestamp %d", (int)timestamp);
    }
}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return s_num_connections > 0 ? s_num_connections : 1;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    static char header[64];
    snprintf(header, sizeof(header), "%s - %s",
             s_connection.departure_station_name,
             s_connection.arrival_station_name);
    menu_cell_basic_header_draw(ctx, cell_layer, header);
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    return 68;  // Taller cells for three-line layout
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Drawing row %d, total connections: %d", cell_index->row, s_num_connections);

    if (s_num_connections == 0) {
        APP_LOG(APP_LOG_LEVEL_INFO, "No connections, showing loading");
        menu_cell_basic_draw(ctx, cell_layer, "Loading...", "Fetching trains", NULL);
        return;
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "Drawing connection %d", cell_index->row);
    Connection *conn = &s_connections[cell_index->row];
    GRect bounds = layer_get_bounds(cell_layer);

    char dep_time[6], arr_time[6];
    format_time(conn->departure_time, dep_time, sizeof(dep_time));
    format_time(conn->arrival_time, arr_time, sizeof(arr_time));
    APP_LOG(APP_LOG_LEVEL_INFO, "Formatted times: %s - %s", dep_time, arr_time);

    // Small header: Train type | Platform
    char header[64];
    if (conn->sections[0].platform[0] != '\0') {
        snprintf(header, sizeof(header), "%s | Pl.%s",
                 conn->sections[0].train_type,
                 conn->sections[0].platform);
    } else {
        snprintf(header, sizeof(header), "%s", conn->sections[0].train_type);
    }

    // Large middle: Time
    char time_text[32];
    if (conn->num_changes > 0) {
        snprintf(time_text, sizeof(time_text), "%s - %s | %d chg",
                 dep_time, arr_time, conn->num_changes);
    } else {
        snprintf(time_text, sizeof(time_text), "%s - %s",
                 dep_time, arr_time);
    }

    // Small footer: Delay status
    char footer[32];
    if (conn->total_delay_minutes > 60) {
        snprintf(footer, sizeof(footer), "+60+ min delay");
    } else if (conn->total_delay_minutes > 0) {
        snprintf(footer, sizeof(footer), "+%d min delay", conn->total_delay_minutes);
    } else {
        snprintf(footer, sizeof(footer), "On time");
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "Drawing text - Header: '%s', Time: '%s', Footer: '%s'",
            header, time_text, footer);

    // Draw three-line layout
    graphics_context_set_text_color(ctx, GColorBlack);

    // Header (small font)
    graphics_draw_text(ctx, header,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      GRect(4, 2, bounds.size.w - 8, 20),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Time (large font)
    graphics_draw_text(ctx, time_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                      GRect(4, 20, bounds.size.w - 8, 28),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Footer (small font)
    graphics_draw_text(ctx, footer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      GRect(4, 46, bounds.size.w - 8, 20),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    APP_LOG(APP_LOG_LEVEL_INFO, "Finished drawing connection row");
}

static void refresh_timer_callback(void *data) {
    request_connections();
    s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void request_connections(void) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Requesting connections: %s → %s",
            s_connection.departure_station_id, s_connection.arrival_station_id);
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_CONNECTIONS, 1);
    dict_write_cstring(iter, MESSAGE_KEY_DEPARTURE_STATION_ID, s_connection.departure_station_id);
    dict_write_cstring(iter, MESSAGE_KEY_ARRIVAL_STATION_ID, s_connection.arrival_station_id);
    app_message_outbox_send();
}

static void window_load(Window *window) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Connection detail window_load called");
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Status layer
    s_status_layer = text_layer_create(GRect(0, bounds.size.h - 20, bounds.size.w, 20));
    text_layer_set_text(s_status_layer, "Updating...");
    text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
    text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

    // Menu layer
    GRect menu_bounds = GRect(0, 0, bounds.size.w, bounds.size.h - 20);
    s_menu_layer = menu_layer_create(menu_bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .get_cell_height = menu_get_cell_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    // Request initial data
    request_connections();

    // Start refresh timer
    s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void window_unload(Window *window) {
    if (s_refresh_timer) {
        app_timer_cancel(s_refresh_timer);
        s_refresh_timer = NULL;
    }
    text_layer_destroy(s_status_layer);
    menu_layer_destroy(s_menu_layer);
}

void connection_detail_window_push(SavedConnection *connection) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Connection detail window push: %s → %s",
            connection->departure_station_name, connection->arrival_station_name);
    s_connection = *connection;
    s_num_connections = 0;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    APP_LOG(APP_LOG_LEVEL_INFO, "Pushing connection detail window");
    window_stack_push(s_window, true);
}

void connection_detail_window_update_data(Connection *connections, int count) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Connection detail update: received %d connections", count);
    s_num_connections = count;
    if (count > 5) s_num_connections = 5;

    for (int i = 0; i < s_num_connections; i++) {
        s_connections[i] = connections[i];
        APP_LOG(APP_LOG_LEVEL_INFO, "Connection %d: dep=%d arr=%d", i,
                (int)connections[i].departure_time, (int)connections[i].arrival_time);
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "Reloading menu layer with %d connections", s_num_connections);
    menu_layer_reload_data(s_menu_layer);
    text_layer_set_text(s_status_layer, "Updated");
}
