#include "connection_detail_window.h"
#include "journey_detail_window.h"
#include "pinned_connection.h"
#include "persistence.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static SavedConnection s_connection;
static Connection s_connections[5];
static int s_num_connections = 0;
static TextLayer *s_status_layer;
static AppTimer *s_refresh_timer;

// Text scrolling state
static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset = 0;
static bool s_scrolling_required = false;
static bool s_menu_reloading = false;

#define REFRESH_INTERVAL_MS 60000  // 60 seconds
#define SCROLL_WAIT_MS 1000
#define SCROLL_STEP_MS 200
#define MENU_CHARS_VISIBLE 14  // Shorter due to time text

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

static void scroll_menu_callback(void *data) {
    s_scroll_timer = NULL;
    s_scroll_offset++;

    if (!s_scrolling_required) {
        return;
    }

    s_menu_reloading = true;
    s_scrolling_required = false;
    menu_layer_reload_data(s_menu_layer);
    s_scroll_timer = app_timer_register(SCROLL_STEP_MS, scroll_menu_callback, NULL);
}

static void initiate_menu_scroll_timer(void) {
    s_scrolling_required = true;
    s_scroll_offset = 0;
    s_menu_reloading = false;

    if (s_scroll_timer) {
        app_timer_cancel(s_scroll_timer);
    }
    s_scroll_timer = app_timer_register(SCROLL_WAIT_MS, scroll_menu_callback, NULL);
}

static void menu_selection_changed_callback(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *data) {
    if (!s_menu_reloading) {
        initiate_menu_scroll_timer();
    } else {
        s_menu_reloading = false;
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

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (s_num_connections == 0) {
        return;  // No connections to select
    }

    if (cell_index->row >= s_num_connections) {
        return;  // Out of bounds
    }

    Connection *selected = &s_connections[cell_index->row];
    journey_detail_window_push(selected);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Drawing row %d, total connections: %d", cell_index->row, s_num_connections);

    if (s_num_connections == 0) {
        APP_LOG(APP_LOG_LEVEL_INFO, "No connections, showing loading");
        menu_cell_basic_draw(ctx, cell_layer, "Loading...", "Fetching trains", NULL);
        return;
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "Drawing connection %d", cell_index->row);

    // Bounds check to prevent crash
    if (cell_index->row >= s_num_connections) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Row %d out of bounds (total: %d)",
                cell_index->row, s_num_connections);
        menu_cell_basic_draw(ctx, cell_layer, "Error", "Invalid row", NULL);
        return;
    }

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

    // Check if selected for scrolling
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (cell_index->row == selected_index.row);

    const char *time_to_draw = time_text;
    if (is_selected) {
        int len = strlen(time_text);
        if (len > MENU_CHARS_VISIBLE) {
            if (s_scroll_offset < len - MENU_CHARS_VISIBLE) {
                time_to_draw += s_scroll_offset;
                s_scrolling_required = true;
            }
        }
    }

    // Fill background with white first
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw three-line layout with black text
    graphics_context_set_text_color(ctx, GColorBlack);

    // Header (small font)
    graphics_draw_text(ctx, header,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      GRect(4, 2, bounds.size.w - 8, 20),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Time (large font)
    graphics_draw_text(ctx, time_to_draw,
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

static TextLayer *s_confirmation_layer = NULL;

static void hide_confirmation(void *data) {
    if (s_confirmation_layer) {
        text_layer_destroy(s_confirmation_layer);
        s_confirmation_layer = NULL;
    }
}

static void show_confirmation(const char *message) {
    Layer *window_layer = window_get_root_layer(s_window);
    GRect bounds = layer_get_bounds(window_layer);

    if (s_confirmation_layer) {
        text_layer_destroy(s_confirmation_layer);
    }

    s_confirmation_layer = text_layer_create(GRect(0, bounds.size.h - 40, bounds.size.w, 40));
    text_layer_set_text(s_confirmation_layer, message);
    text_layer_set_text_alignment(s_confirmation_layer, GTextAlignmentCenter);
    text_layer_set_font(s_confirmation_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_background_color(s_confirmation_layer, GColorBlack);
    text_layer_set_text_color(s_confirmation_layer, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(s_confirmation_layer));

    app_timer_register(2000, hide_confirmation, NULL);
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Save connection route
    SavedConnection new_connection = create_saved_connection(
        s_connection.departure_station_id,
        s_connection.departure_station_name,
        s_connection.arrival_station_id,
        s_connection.arrival_station_name
    );

    SavedConnection connections[MAX_SAVED_CONNECTIONS];
    int count = load_connections(connections);

    if (count < MAX_SAVED_CONNECTIONS) {
        connections[count++] = new_connection;
        save_connections(connections, count);
        show_confirmation("Connection saved");
        APP_LOG(APP_LOG_LEVEL_INFO, "Saved connection: %s -> %s",
                new_connection.departure_station_name, new_connection.arrival_station_name);
    } else {
        show_confirmation("Max connections reached");
        APP_LOG(APP_LOG_LEVEL_WARNING, "Cannot save: max connections reached");
    }
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    // Pin current connection
    MenuIndex selected = menu_layer_get_selected_index(s_menu_layer);

    if (selected.row >= s_num_connections) {
        return;
    }

    Connection *conn = &s_connections[selected.row];

    PinnedConnection pinned;
    pinned.connection = *conn;
    pinned.route = s_connection;  // SavedConnection with station info
    pinned.pinned_at = time(NULL);
    pinned.is_active = true;

    save_pinned_connection(&pinned);
    show_confirmation("Connection pinned");
    APP_LOG(APP_LOG_LEVEL_INFO, "Pinned connection: %s -> %s, arrival: %d",
            pinned.route.departure_station_name, pinned.route.arrival_station_name,
            (int)pinned.connection.arrival_time);
}

static void click_config_provider(void *context) {
    window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
    window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
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
        .select_click = menu_select_callback,
        .selection_changed = menu_selection_changed_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);

    // Set menu colors explicitly
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);

    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    APP_LOG(APP_LOG_LEVEL_INFO, "Menu layer configured with colors");

    // Register click config provider for long-press handlers
    window_set_click_config_provider(s_window, click_config_provider);

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
    if (s_scroll_timer) {
        app_timer_cancel(s_scroll_timer);
        s_scroll_timer = NULL;
    }
    if (s_confirmation_layer) {
        text_layer_destroy(s_confirmation_layer);
        s_confirmation_layer = NULL;
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
