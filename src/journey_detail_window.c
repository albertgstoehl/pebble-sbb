#include "journey_detail_window.h"
#include "pinned_connection.h"
#include "persistence.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static Connection s_connection;

static TextLayer *s_confirmation_layer = NULL;

// Text scrolling state
static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset = 0;
static bool s_scrolling_required = false;
static bool s_menu_reloading = false;

#define SCROLL_WAIT_MS 1000
#define SCROLL_STEP_MS 200
#define MENU_CHARS_VISIBLE 17

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
    // Save connection route - extract from first and last sections
    SavedConnection new_connection = create_saved_connection(
        "",  // Station IDs not available in journey sections
        s_connection.sections[0].departure_station,
        "",
        s_connection.sections[s_connection.num_sections - 1].arrival_station
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
    PinnedConnection pinned;
    pinned.connection = s_connection;

    // Extract route info from first and last sections
    SavedConnection route;
    snprintf(route.departure_station_id, sizeof(route.departure_station_id), "%s", "");
    snprintf(route.departure_station_name, sizeof(route.departure_station_name), "%s",
             s_connection.sections[0].departure_station);
    snprintf(route.arrival_station_id, sizeof(route.arrival_station_id), "%s", "");
    snprintf(route.arrival_station_name, sizeof(route.arrival_station_name), "%s",
             s_connection.sections[s_connection.num_sections - 1].arrival_station);

    pinned.route = route;
    pinned.pinned_at = time(NULL);
    pinned.is_active = true;

    save_pinned_connection(&pinned);
    show_confirmation("Connection pinned");
    APP_LOG(APP_LOG_LEVEL_INFO, "Pinned connection from journey detail");
}

static void click_config_provider(void *context) {
    window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
    window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
}

// Scroll timer callbacks
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
        char title[64];
        char subtitle[32];

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

    // Section row - custom graphics drawing
    int section_idx = cell_index->row - 1;
    if (section_idx < 0 || section_idx >= s_connection.num_sections) return;
    JourneySection *section = &s_connection.sections[section_idx];
    GRect bounds = layer_get_bounds(cell_layer);

    // Fill background
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorBlack);

    // Format times
    char dep_time[6], arr_time[6];
    struct tm *dep_tm = localtime(&section->departure_time);
    struct tm *arr_tm = localtime(&section->arrival_time);

    if (dep_tm && arr_tm) {
        strftime(dep_time, sizeof(dep_time), "%H:%M", dep_tm);
        strftime(arr_time, sizeof(arr_time), "%H:%M", arr_tm);
    } else {
        snprintf(dep_time, sizeof(dep_time), "??:??");
        snprintf(arr_time, sizeof(arr_time), "??:??");
    }

    // Graphics positioning
    GPoint circle_center = GPoint(8, 10);
    int circle_radius = 4;
    int line_start_y = circle_center.y + circle_radius;
    int line_end_y = bounds.size.h - 10;

    // Draw departure circle (filled)
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, circle_center, circle_radius);

    // Draw vertical connecting line (except for last section)
    if (section_idx < s_connection.num_sections - 1) {
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_line(ctx, GPoint(circle_center.x, line_start_y),
                          GPoint(circle_center.x, line_end_y));
    }

    // Check if selected for scrolling
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (cell_index->row == selected_index.row);

    // Draw departure station + platform
    char dep_text[48];
    const char *dep_to_draw = section->departure_station;

    if (section->platform[0] != '\0') {
        snprintf(dep_text, sizeof(dep_text), "%s | Pl.%s",
                 section->departure_station, section->platform);
        dep_to_draw = dep_text;
    }

    // Apply scroll offset if selected
    if (is_selected) {
        int len = strlen(dep_to_draw);
        if (len > MENU_CHARS_VISIBLE) {
            if (s_scroll_offset < len - MENU_CHARS_VISIBLE) {
                dep_to_draw += s_scroll_offset;
                s_scrolling_required = true;
            }
        }
    }

    graphics_draw_text(ctx, dep_to_draw,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      GRect(18, 0, bounds.size.w - 20, 20),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Draw train type + departure time
    char train_text[32];
    snprintf(train_text, sizeof(train_text), "%s → %s",
             section->train_type, dep_time);

    graphics_draw_text(ctx, train_text,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                      GRect(18, 18, bounds.size.w - 20, 20),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Draw arrival station
    circle_center = GPoint(8, 42);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_circle(ctx, circle_center, circle_radius);

    char arr_text[48];
    const char *arr_to_draw = section->arrival_station;
    snprintf(arr_text, sizeof(arr_text), "%s", section->arrival_station);
    arr_to_draw = arr_text;

    // Apply scroll offset if selected
    if (is_selected) {
        int len = strlen(arr_to_draw);
        if (len > MENU_CHARS_VISIBLE) {
            if (s_scroll_offset < len - MENU_CHARS_VISIBLE) {
                arr_to_draw += s_scroll_offset;
                s_scrolling_required = true;
            }
        }
    }

    graphics_draw_text(ctx, arr_to_draw,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18),
                      GRect(18, 36, bounds.size.w - 20, 20),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);

    // Draw arrival time + delay
    char arr_info[32];
    if (section->delay_minutes > 0) {
        snprintf(arr_info, sizeof(arr_info), "Arr: %s | +%d min",
                 arr_time, section->delay_minutes);
    } else {
        snprintf(arr_info, sizeof(arr_info), "Arr: %s", arr_time);
    }

    graphics_draw_text(ctx, arr_info,
                      fonts_get_system_font(FONT_KEY_GOTHIC_14),
                      GRect(18, 52, bounds.size.w - 20, 16),
                      GTextOverflowModeTrailingEllipsis,
                      GTextAlignmentLeft,
                      NULL);
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
        .selection_changed = menu_selection_changed_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
    menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    // Register click config provider for long-press handlers
    window_set_click_config_provider(s_window, click_config_provider);
}

static void window_unload(Window *window) {
    if (s_scroll_timer) {
        app_timer_cancel(s_scroll_timer);
        s_scroll_timer = NULL;
    }
    if (s_confirmation_layer) {
        text_layer_destroy(s_confirmation_layer);
        s_confirmation_layer = NULL;
    }
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
