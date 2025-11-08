#include "error_dialog.h"

static Window *s_error_window;
static TextLayer *s_title_layer;
static TextLayer *s_message_layer;

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_title_layer = text_layer_create(GRect(5, 20, bounds.size.w - 10, 40));
    text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

    s_message_layer = text_layer_create(GRect(5, 70, bounds.size.w - 10, 80));
    text_layer_set_font(s_message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_message_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_message_layer));
}

static void window_unload(Window *window) {
    text_layer_destroy(s_title_layer);
    text_layer_destroy(s_message_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    window_stack_pop(true);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

void show_error_dialog(const char *title, const char *message) {
    if (!s_error_window) {
        s_error_window = window_create();
        window_set_window_handlers(s_error_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
        window_set_click_config_provider(s_error_window, click_config_provider);
    }

    window_stack_push(s_error_window, true);
    text_layer_set_text(s_title_layer, title);
    text_layer_set_text(s_message_layer, message);
}
