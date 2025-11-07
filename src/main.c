#include <pebble.h>

static void init(void) {
}

static void deinit(void) {
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
