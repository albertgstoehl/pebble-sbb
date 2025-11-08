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
    strftime(buffer, size, "%H:%M", tm_info);
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
    snprintf(header, sizeof(header), "%s → %s",
             s_connection.departure_station_name,
             s_connection.arrival_station_name);
    menu_cell_basic_header_draw(ctx, cell_layer, header);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    if (s_num_connections == 0) {
        menu_cell_basic_draw(ctx, cell_layer, "Loading...", "Fetching trains", NULL);
        return;
    }

    Connection *conn = &s_connections[cell_index->row];
    static char title[64];
    static char subtitle[64];

    char dep_time[6], arr_time[6];
    format_time(conn->departure_time, dep_time, sizeof(dep_time));
    format_time(conn->arrival_time, arr_time, sizeof(arr_time));

    // Format: "14:32 → 15:47 | 1 chg | Pl.7"
    if (conn->num_changes > 0) {
        snprintf(title, sizeof(title), "%s → %s | %d chg",
                 dep_time, arr_time, conn->num_changes);
    } else {
        snprintf(title, sizeof(title), "%s → %s | Direct",
                 dep_time, arr_time);
    }

    // Format subtitle with delay info
    if (conn->sections[0].platform[0] != '\0') {
        if (conn->total_delay_minutes > 0) {
            snprintf(subtitle, sizeof(subtitle), "Pl.%s | %s | +%d min",
                     conn->sections[0].platform,
                     conn->sections[0].train_type,
                     conn->total_delay_minutes);
        } else {
            snprintf(subtitle, sizeof(subtitle), "Pl.%s | %s | ✓",
                     conn->sections[0].platform,
                     conn->sections[0].train_type);
        }
    }

    menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void refresh_timer_callback(void *data) {
    request_connections();
    s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void request_connections(void) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_CONNECTIONS, 1);
    dict_write_cstring(iter, MESSAGE_KEY_DEPARTURE_STATION_ID, s_connection.departure_station_id);
    dict_write_cstring(iter, MESSAGE_KEY_ARRIVAL_STATION_ID, s_connection.arrival_station_id);
    app_message_outbox_send();
}

static void window_load(Window *window) {
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
    s_connection = *connection;
    s_num_connections = 0;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}

void connection_detail_window_update_data(Connection *connections, int count) {
    s_num_connections = count;
    if (count > 5) s_num_connections = 5;

    for (int i = 0; i < s_num_connections; i++) {
        s_connections[i] = connections[i];
    }

    menu_layer_reload_data(s_menu_layer);
    text_layer_set_text(s_status_layer, "Updated");
}
