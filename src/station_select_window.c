#include "station_select_window.h"
#include "persistence.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Station s_stations[50];
static int s_num_stations = 0;
static Station s_favorites[MAX_FAVORITE_STATIONS];
static int s_num_favorites = 0;
static StationSelectCallback s_callback;
static TextLayer *s_status_layer;

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return s_num_favorites > 0 ? 2 : 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    if (section_index == 0 && s_num_favorites > 0) {
        return s_num_favorites;
    }
    return s_num_stations > 0 ? s_num_stations : 1;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    if (section_index == 0 && s_num_favorites > 0) {
        menu_cell_basic_header_draw(ctx, cell_layer, "★ Favorites");
    } else {
        menu_cell_basic_header_draw(ctx, cell_layer, "Nearby Stations");
    }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    Station *station;
    static char subtitle[32];

    if (cell_index->section == 0 && s_num_favorites > 0) {
        station = &s_favorites[cell_index->row];
        menu_cell_basic_draw(ctx, cell_layer, station->name, NULL, NULL);
    } else {
        if (s_num_stations == 0) {
            menu_cell_basic_draw(ctx, cell_layer, "Loading...", "Searching GPS", NULL);
            return;
        }
        station = &s_stations[cell_index->row];
        snprintf(subtitle, sizeof(subtitle), "%.1f km", station->distance_meters / 1000.0);
        menu_cell_basic_draw(ctx, cell_layer, station->name, subtitle, NULL);
    }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    Station *selected;

    if (cell_index->section == 0 && s_num_favorites > 0) {
        selected = &s_favorites[cell_index->row];
    } else {
        if (s_num_stations == 0) return;
        selected = &s_stations[cell_index->row];
    }

    if (s_callback) {
        s_callback(selected);
    }
    window_stack_pop(true);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Create status layer
    s_status_layer = text_layer_create(GRect(0, bounds.size.h - 30, bounds.size.w, 30));
    text_layer_set_text(s_status_layer, "Searching nearby...");
    text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
    text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

    // Create menu layer
    GRect menu_bounds = GRect(0, 0, bounds.size.w, bounds.size.h - 30);
    s_menu_layer = menu_layer_create(menu_bounds);
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

    // Load favorites
    s_num_favorites = load_favorites(s_favorites);

    // Request nearby stations via AppMessage
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_NEARBY_STATIONS, 1);
    app_message_outbox_send();
}

static void window_unload(Window *window) {
    text_layer_destroy(s_status_layer);
    menu_layer_destroy(s_menu_layer);
}

void station_select_window_push(StationSelectCallback callback) {
    s_callback = callback;
    s_num_stations = 0;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}

void station_select_window_add_station(Station station) {
    if (s_num_stations < 50) {
        s_stations[s_num_stations++] = station;
        menu_layer_reload_data(s_menu_layer);

        static char status[32];
        snprintf(status, sizeof(status), "Found %d stations", s_num_stations);
        text_layer_set_text(s_status_layer, status);
    }
}

void station_select_window_clear_stations(void) {
    s_num_stations = 0;
    menu_layer_reload_data(s_menu_layer);
}
