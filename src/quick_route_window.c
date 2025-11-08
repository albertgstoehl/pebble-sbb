#include "quick_route_window.h"
#include "persistence.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Station s_departure_station;
static FavoriteDestination s_favorites[MAX_FAVORITE_DESTINATIONS];
static int s_num_favorites = 0;
static QuickRouteCallback s_callback;

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return s_num_favorites > 0 ? s_num_favorites : 1;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Travel to...");
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    if (s_num_favorites == 0) {
        menu_cell_basic_draw(ctx, cell_layer,
                           "No favorites",
                           "Configure in Pebble app settings",
                           NULL);
        return;
    }

    FavoriteDestination *fav = &s_favorites[cell_index->row];
    menu_cell_basic_draw(ctx, cell_layer, fav->label, fav->name, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (s_num_favorites == 0) {
        return;
    }

    FavoriteDestination *fav = &s_favorites[cell_index->row];

    APP_LOG(APP_LOG_LEVEL_INFO, "Quick route destination selected: %s, popping window", fav->label);

    // Pop this window first
    APP_LOG(APP_LOG_LEVEL_INFO, "Popping quick route window");
    window_stack_pop(true);

    // Then call callback to push connection detail window
    if (s_callback) {
        s_callback(&s_departure_station, fav);
    }
}

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

    // Load favorites
    s_num_favorites = load_favorite_destinations(s_favorites);
    APP_LOG(APP_LOG_LEVEL_INFO, "Quick route window loaded with %d favorites", s_num_favorites);
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
}

void quick_route_window_push(Station *departure_station, QuickRouteCallback callback) {
    s_departure_station = *departure_station;
    s_callback = callback;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}
