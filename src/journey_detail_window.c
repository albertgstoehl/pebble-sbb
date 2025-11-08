#include "journey_detail_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Connection s_connection;

// Menu callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // +1 for header row showing overall journey summary
    return s_connection.num_sections + 1;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Journey Details");
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (cell_index->row == 0) {
        return 44;  // Header row: overall summary
    }
    return 70;  // Section rows: taller for graphics
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    if (cell_index->row == 0) {
        // Overall journey summary
        static char title[64];
        static char subtitle[32];

        char dep_time[6], arr_time[6];
        struct tm *dep_tm = localtime(&s_connection.departure_time);
        struct tm *arr_tm = localtime(&s_connection.arrival_time);

        if (dep_tm && arr_tm) {
            strftime(dep_time, sizeof(dep_time), "%H:%M", dep_tm);
            strftime(arr_time, sizeof(arr_time), "%H:%M", arr_tm);
        } else {
            snprintf(dep_time, sizeof(dep_time), "??:??");
            snprintf(arr_time, sizeof(arr_time), "??:??");
        }

        snprintf(title, sizeof(title), "%s - %s", dep_time, arr_time);
        snprintf(subtitle, sizeof(subtitle), "%d changes", s_connection.num_changes);

        menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
        return;
    }

    // Section row - for now just show basic text
    int section_idx = cell_index->row - 1;
    JourneySection *section = &s_connection.sections[section_idx];

    menu_cell_basic_draw(ctx, cell_layer, section->departure_station, section->train_type, NULL);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .get_cell_height = menu_get_cell_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
}

void journey_detail_window_push(Connection *connection) {
    s_connection = *connection;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}
