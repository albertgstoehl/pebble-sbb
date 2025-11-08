#include "station_select_window.h"
#include "persistence.h"

#define SCROLL_WAIT_MS 1000  // Wait 1 second before starting scroll
#define SCROLL_STEP_MS 200   // Scroll every 200ms
#define MENU_CHARS_VISIBLE 17 // Approx chars visible in menu cell

static Window *s_window;
static MenuLayer *s_menu_layer;
static Station s_stations[50];
static int s_num_stations = 0;
static Station s_favorites[MAX_FAVORITE_STATIONS];
static int s_num_favorites = 0;
static StationSelectCallback s_callback;
static TextLayer *s_status_layer;
static bool s_gps_search_active = false;

// Text scrolling state
static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset = 0;
static bool s_scrolling_required = false;
static bool s_menu_reloading = false;

// Forward declarations
static void scroll_menu_callback(void *data);
static void initiate_menu_scroll_timer(void);

// Convert FavoriteDestination to Station for display
static Station favorite_to_station(FavoriteDestination *fav) {
    return create_station(fav->id, fav->name, 0);  // distance = 0 for favorites
}

// Timer-based text scrolling implementation
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

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    // Always show 2 sections: Action/Nearby + Favorites
    return 2;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    if (section_index == 0) {
        // Section 0: "Stations near me" or GPS results
        if (!s_gps_search_active && s_num_stations == 0) {
            return 1;  // Just "Stations near me" row
        }
        return s_num_stations > 0 ? s_num_stations : 1;  // GPS results or loading
    }
    // Section 1: Favorites
    return s_num_favorites;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    if (section_index == 0) {
        if (s_num_stations > 0) {
            menu_cell_basic_header_draw(ctx, cell_layer, "Nearby Stations");
        } else {
            menu_cell_basic_header_draw(ctx, cell_layer, "Select Departure");
        }
    } else {
        menu_cell_basic_header_draw(ctx, cell_layer, "Favorites");
    }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    Station *station;
    static char subtitle[32];
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (cell_index->section == selected_index.section &&
                        cell_index->row == selected_index.row);

    if (cell_index->section == 0) {
        // Section 0: "Stations near me" or GPS results
        if (!s_gps_search_active && s_num_stations == 0) {
            // Show "Stations near me" trigger
            menu_cell_basic_draw(ctx, cell_layer, "Stations near me", "Search by GPS", NULL);
            return;
        }

        if (s_num_stations == 0) {
            // GPS search active but no results yet
            menu_cell_basic_draw(ctx, cell_layer, "Searching...", "Finding nearby stations", NULL);
            return;
        }

        // Show GPS results
        station = &s_stations[cell_index->row];

        // Format distance
        if (station->distance_meters < 1000) {
            snprintf(subtitle, sizeof(subtitle), "%d m", station->distance_meters);
        } else {
            int km = station->distance_meters / 1000;
            int decimal = (station->distance_meters % 1000) / 100;
            snprintf(subtitle, sizeof(subtitle), "%d.%d km", km, decimal);
        }

        const char *name_to_draw = station->name;

        // Apply scroll offset if selected
        if (is_selected) {
            int len = strlen(station->name);
            if (len - MENU_CHARS_VISIBLE - s_scroll_offset > 0) {
                name_to_draw += s_scroll_offset;
                s_scrolling_required = true;
            }
        }

        menu_cell_basic_draw(ctx, cell_layer, name_to_draw, subtitle, NULL);
    } else {
        // Section 1: Favorites
        if (s_num_favorites == 0) {
            menu_cell_basic_draw(ctx, cell_layer,
                               "No favorites",
                               "Configure in Pebble app",
                               NULL);
            return;
        }

        station = &s_favorites[cell_index->row];
        const char *name_to_draw = station->name;

        // Apply scroll offset if selected
        if (is_selected) {
            int len = strlen(station->name);
            if (len - MENU_CHARS_VISIBLE - s_scroll_offset > 0) {
                name_to_draw += s_scroll_offset;
                s_scrolling_required = true;
            }
        }

        menu_cell_basic_draw(ctx, cell_layer, name_to_draw, NULL, NULL);
    }
}

static void menu_selection_changed_callback(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *data) {
    // Start scroll timer when selection changes
    if (!s_menu_reloading) {
        initiate_menu_scroll_timer();
    } else {
        s_menu_reloading = false;
    }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    Station *selected;

    if (cell_index->section == 0) {
        // Section 0: "Stations near me" or GPS results
        if (!s_gps_search_active && s_num_stations == 0) {
            // User selected "Stations near me" - trigger GPS
            s_gps_search_active = true;
            menu_layer_reload_data(s_menu_layer);

            // Request nearby stations via AppMessage
            DictionaryIterator *iter;
            app_message_outbox_begin(&iter);
            dict_write_uint8(iter, MESSAGE_KEY_REQUEST_NEARBY_STATIONS, 1);
            app_message_outbox_send();

            text_layer_set_text(s_status_layer, "Searching nearby...");
            return;
        }

        if (s_num_stations == 0) {
            // Still loading, ignore selection
            return;
        }

        // GPS result selected
        selected = &s_stations[cell_index->row];
    } else {
        // Section 1: Favorite selected
        if (s_num_favorites == 0) {
            return;
        }
        selected = &s_favorites[cell_index->row];
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
    text_layer_set_text(s_status_layer, "Select a station");
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
        .selection_changed = menu_selection_changed_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    // Enable normal colors (helps with text rendering)
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    // Load favorites and convert to Station format
    FavoriteDestination fav_destinations[MAX_FAVORITE_DESTINATIONS];
    int num_fav_destinations = load_favorite_destinations(fav_destinations);

    s_num_favorites = 0;
    for (int i = 0; i < num_fav_destinations && i < MAX_FAVORITE_STATIONS; i++) {
        s_favorites[s_num_favorites++] = favorite_to_station(&fav_destinations[i]);
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "Loaded %d favorite destinations", s_num_favorites);

    // DO NOT automatically trigger GPS - wait for user to select "Stations near me"
}

static void window_unload(Window *window) {
    if (s_scroll_timer) {
        app_timer_cancel(s_scroll_timer);
        s_scroll_timer = NULL;
    }
    text_layer_destroy(s_status_layer);
    menu_layer_destroy(s_menu_layer);
}

void station_select_window_push(StationSelectCallback callback) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Station select window push called");
    s_callback = callback;
    s_num_stations = 0;
    s_gps_search_active = false;  // Reset GPS search state

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
