#include "main_window.h"
#include "data_models.h"
#include "persistence.h"
#include "add_connection_window.h"
#include "connection_detail_window.h"
#include "quick_route_window.h"
#include "station_select_window.h"
#include "pinned_connection.h"
#include "journey_detail_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static SavedConnection s_connections[MAX_SAVED_CONNECTIONS];
static int s_num_connections = 0;
static PinnedConnection s_pinned_connection;
static bool s_has_pinned = false;

// Forward declarations
static void quick_route_selected_callback(Station *departure, FavoriteDestination *destination);
static void quick_route_departure_selected(Station *station);
static void start_quick_route(void);

// Menu callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return s_has_pinned ? 2 : 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    if (s_has_pinned && section_index == 0) {
        return 1;  // Active Journey section
    }

    int saved_section = s_has_pinned ? 1 : 0;
    if (section_index == saved_section) {
        return s_num_connections > 0 ? s_num_connections : 1;
    }

    return 0;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    if (s_has_pinned && section_index == 0) {
        menu_cell_basic_header_draw(ctx, cell_layer, "Active Journey");
    } else {
        menu_cell_basic_header_draw(ctx, cell_layer, "Saved Connections");
    }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    if (s_has_pinned && cell_index->section == 0) {
        // Active Journey row
        static char title[64];
        static char subtitle[32];

        char dep_time[6];
        struct tm *dep_tm = localtime(&s_pinned_connection.connection.departure_time);
        if (dep_tm) {
            strftime(dep_time, sizeof(dep_time), "%H:%M", dep_tm);
        } else {
            snprintf(dep_time, sizeof(dep_time), "??:??");
        }

        snprintf(title, sizeof(title), "%s → %s",
                 s_pinned_connection.route.departure_station_name,
                 s_pinned_connection.route.arrival_station_name);
        snprintf(subtitle, sizeof(subtitle), "Departs at %s", dep_time);

        menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
        return;
    }

    // Saved Connections section
    if (s_num_connections == 0) {
        menu_cell_basic_draw(ctx, cell_layer, "Add Connection", "Long-press UP", NULL);
    } else {
        SavedConnection *conn = &s_connections[cell_index->row];
        static char title[64];
        snprintf(title, sizeof(title), "%s → %s",
                 conn->departure_station_name,
                 conn->arrival_station_name);
        menu_cell_basic_draw(ctx, cell_layer, title, "Tap to view trains", NULL);
    }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (s_has_pinned && cell_index->section == 0) {
        // Open pinned journey detail
        journey_detail_window_push(&s_pinned_connection.connection);
        return;
    }

    // Saved connections section
    if (s_num_connections == 0) {
        add_connection_window_push();
        return;
    }
    SavedConnection *conn = &s_connections[cell_index->row];
    connection_detail_window_push(conn);
}

// Quick route functions
static void quick_route_selected_callback(Station *departure, FavoriteDestination *destination) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Quick route: %s to %s",
            departure->name, destination->label);

    // Create temporary saved connection for display
    SavedConnection temp_connection = create_saved_connection(
        departure->id,
        departure->name,
        destination->id,
        destination->name
    );

    // Push connection detail window (will request data via AppMessage)
    connection_detail_window_push(&temp_connection);
}

static void quick_route_departure_selected(Station *station) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Departure selected: %s", station->name);
    quick_route_window_push(station, quick_route_selected_callback);
    APP_LOG(APP_LOG_LEVEL_INFO, "Quick route window pushed");
}

static void start_quick_route(void) {
    station_select_window_push(quick_route_departure_selected);
}

// Click config provider
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    start_quick_route();
}

static void click_config_provider(void *context) {
    window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);
}

// Window lifecycle
static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Load and check pinned connection
    s_pinned_connection = load_pinned_connection();

    if (s_pinned_connection.is_active && is_pinned_connection_expired(&s_pinned_connection)) {
        clear_pinned_connection();
        s_pinned_connection.is_active = false;
    }

    s_has_pinned = s_pinned_connection.is_active;
    APP_LOG(APP_LOG_LEVEL_INFO, "Main window: has_pinned=%d", s_has_pinned);

    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    // Register long-press handler for quick route
    window_set_click_config_provider(s_window, click_config_provider);

    // Load saved connections
    s_num_connections = load_connections(s_connections);
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
}

void main_window_push(void) {
    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}

void main_window_pop(void) {
    window_stack_remove(s_window, true);
}

void main_window_refresh(void) {
    s_num_connections = load_connections(s_connections);
    menu_layer_reload_data(s_menu_layer);
}
