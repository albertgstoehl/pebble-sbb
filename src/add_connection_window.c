#include "add_connection_window.h"
#include "station_select_window.h"
#include "persistence.h"
#include "main_window.h"

static Window *s_window;
static TextLayer *s_instruction_layer;
static Station s_departure_station;
static Station s_arrival_station;
static bool s_departure_selected = false;

static void save_connection(void) {
    SavedConnection connections[MAX_SAVED_CONNECTIONS];
    int count = load_connections(connections);

    if (count >= MAX_SAVED_CONNECTIONS) {
        // TODO: Show error
        return;
    }

    connections[count] = create_saved_connection(
        s_departure_station.id,
        s_departure_station.name,
        s_arrival_station.id,
        s_arrival_station.name
    );

    save_connections(connections, count + 1);
    main_window_refresh();
    window_stack_pop(true);
}

static void arrival_selected_callback(Station *station) {
    s_arrival_station = *station;
    save_connection();
}

static void departure_selected_callback(Station *station) {
    s_departure_station = *station;
    s_departure_selected = true;

    // Push station select for arrival immediately
    // (the instruction window will be hidden automatically when new window pushes)
    station_select_window_push(arrival_selected_callback);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (!s_departure_selected) {
        station_select_window_push(departure_selected_callback);
    }
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_instruction_layer = text_layer_create(GRect(0, 60, bounds.size.w, 60));
    text_layer_set_text(s_instruction_layer, "Press SELECT to choose departure station");
    text_layer_set_text_alignment(s_instruction_layer, GTextAlignmentCenter);
    text_layer_set_font(s_instruction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(s_instruction_layer));
}

static void window_unload(Window *window) {
    text_layer_destroy(s_instruction_layer);
}

void add_connection_window_push(void) {
    s_departure_selected = false;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
        window_set_click_config_provider(s_window, click_config_provider);
    }
    window_stack_push(s_window, true);
}
