#include "main_window.h"
#include "data_models.h"
#include "persistence.h"
#include "add_connection_window.h"
#include "connection_detail_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static SavedConnection s_connections[MAX_SAVED_CONNECTIONS];
static int s_num_connections = 0;

// Menu callbacks
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
    menu_cell_basic_header_draw(ctx, cell_layer, "Saved Connections");
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
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
    if (s_num_connections == 0) {
        add_connection_window_push();
        return;
    }
    SavedConnection *conn = &s_connections[cell_index->row];
    connection_detail_window_push(conn);
}

// Window lifecycle
static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

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
