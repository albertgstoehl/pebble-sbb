#include <pebble.h>
#include "main_window.h"
#include "app_message.h"

static void init(void) {
    app_message_init();
    main_window_push();
}

static void deinit(void) {
    main_window_pop();
    app_message_deinit();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
