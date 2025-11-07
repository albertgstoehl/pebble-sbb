# SBB Train Schedule Pebble App Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a Pebble watch app that displays Swiss train schedules with saved connections, GPS-based station selection, and real-time delay information.

**Architecture:** PebbleKit JS handles networking (SBB OpenData API) and GPS on phone; C handles UI rendering, persistence, and user interaction on watch; AppMessage protocol for communication.

**Tech Stack:** C (Pebble SDK), JavaScript (PebbleKit JS), SBB OpenData API, Pebble SDK 4.x, QEMU emulator (aplite platform)

---

## Prerequisites

### Task 0: Development Environment Setup

**Files:**
- Create: `package.json`
- Create: `appinfo.json`
- Create: `wscript`

**Step 1: Install Pebble SDK**

```bash
pip install pebble-tool
pebble sdk install
```

Expected: SDK installed successfully, `pebble` command available

**Step 2: Initialize Pebble project**

```bash
pebble new-project --simple sbb-schedule
cd sbb-schedule
```

Expected: Project structure created with src/, resources/, package.json

**Step 3: Configure appinfo.json**

Edit `appinfo.json`:

```json
{
  "uuid": "8a9b4c2d-1e3f-4a5b-6c7d-8e9f0a1b2c3d",
  "shortName": "SBB Schedule",
  "longName": "SBB Train Schedule",
  "companyName": "YourName",
  "versionLabel": "1.0",
  "sdkVersion": "3",
  "targetPlatforms": ["aplite", "basalt"],
  "watchapp": {
    "watchface": false
  },
  "appKeys": {
    "REQUEST_CONNECTIONS": 0,
    "REQUEST_NEARBY_STATIONS": 1,
    "CONNECTION_DATA": 2,
    "STATION_LIST": 3,
    "DEPARTURE_STATION_ID": 10,
    "ARRIVAL_STATION_ID": 11,
    "STATION_NAME": 20,
    "STATION_ID": 21,
    "STATION_DISTANCE": 22,
    "DEPARTURE_TIME": 30,
    "ARRIVAL_TIME": 31,
    "PLATFORM": 32,
    "TRAIN_TYPE": 33,
    "DELAY_MINUTES": 34,
    "NUM_CHANGES": 35,
    "ERROR_MESSAGE": 40
  },
  "resources": {
    "media": []
  }
}
```

**Step 4: Verify build**

```bash
pebble build
```

Expected: BUILD SUCCESSFUL

**Step 5: Commit initial setup**

```bash
git add .
git commit -m "feat: initialize Pebble project structure"
```

---

## Phase 1: Data Models and Persistence

### Task 1: Define Data Structures

**Files:**
- Create: `src/data_models.h`
- Create: `src/data_models.c`

**Step 1: Write failing test for connection structure**

Create `tests/test_data_models.c`:

```c
#include <check.h>
#include "../src/data_models.h"

START_TEST(test_create_saved_connection) {
    SavedConnection conn = create_saved_connection(
        "8503000", "Zürich HB",
        "8507000", "Bern"
    );
    ck_assert_str_eq(conn.departure_station_id, "8503000");
    ck_assert_str_eq(conn.departure_station_name, "Zürich HB");
    ck_assert_str_eq(conn.arrival_station_id, "8507000");
    ck_assert_str_eq(conn.arrival_station_name, "Bern");
}
END_TEST
```

**Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL with "data_models.h not found"

**Step 3: Define data structures in header**

Create `src/data_models.h`:

```c
#pragma once
#include <pebble.h>

#define MAX_STATION_NAME_LENGTH 32
#define MAX_STATION_ID_LENGTH 16
#define MAX_TRAIN_TYPE_LENGTH 16
#define MAX_PLATFORM_LENGTH 8
#define MAX_SAVED_CONNECTIONS 10
#define MAX_FAVORITE_STATIONS 20

// Station structure
typedef struct {
    char id[MAX_STATION_ID_LENGTH];
    char name[MAX_STATION_NAME_LENGTH];
    int distance_meters;
} Station;

// Saved connection structure
typedef struct {
    char departure_station_id[MAX_STATION_ID_LENGTH];
    char departure_station_name[MAX_STATION_NAME_LENGTH];
    char arrival_station_id[MAX_STATION_ID_LENGTH];
    char arrival_station_name[MAX_STATION_NAME_LENGTH];
} SavedConnection;

// Journey section (leg) structure
typedef struct {
    char departure_station[MAX_STATION_NAME_LENGTH];
    char arrival_station[MAX_STATION_NAME_LENGTH];
    time_t departure_time;
    time_t arrival_time;
    char platform[MAX_PLATFORM_LENGTH];
    char train_type[MAX_TRAIN_TYPE_LENGTH];
    int delay_minutes;
} JourneySection;

// Full connection structure (with sections)
typedef struct {
    JourneySection sections[5];  // Max 5 legs
    int num_sections;
    time_t departure_time;
    time_t arrival_time;
    int total_delay_minutes;
    int num_changes;
} Connection;

// Function declarations
SavedConnection create_saved_connection(
    const char *dep_id, const char *dep_name,
    const char *arr_id, const char *arr_name
);
Station create_station(const char *id, const char *name, int distance);
```

**Step 4: Implement functions**

Create `src/data_models.c`:

```c
#include "data_models.h"
#include <string.h>

SavedConnection create_saved_connection(
    const char *dep_id, const char *dep_name,
    const char *arr_id, const char *arr_name
) {
    SavedConnection conn;
    strncpy(conn.departure_station_id, dep_id, MAX_STATION_ID_LENGTH - 1);
    conn.departure_station_id[MAX_STATION_ID_LENGTH - 1] = '\0';
    strncpy(conn.departure_station_name, dep_name, MAX_STATION_NAME_LENGTH - 1);
    conn.departure_station_name[MAX_STATION_NAME_LENGTH - 1] = '\0';
    strncpy(conn.arrival_station_id, arr_id, MAX_STATION_ID_LENGTH - 1);
    conn.arrival_station_id[MAX_STATION_ID_LENGTH - 1] = '\0';
    strncpy(conn.arrival_station_name, arr_name, MAX_STATION_NAME_LENGTH - 1);
    conn.arrival_station_name[MAX_STATION_NAME_LENGTH - 1] = '\0';
    return conn;
}

Station create_station(const char *id, const char *name, int distance) {
    Station station;
    strncpy(station.id, id, MAX_STATION_ID_LENGTH - 1);
    station.id[MAX_STATION_ID_LENGTH - 1] = '\0';
    strncpy(station.name, name, MAX_STATION_NAME_LENGTH - 1);
    station.name[MAX_STATION_NAME_LENGTH - 1] = '\0';
    station.distance_meters = distance;
    return station;
}
```

**Step 5: Run test to verify it passes**

Run: `make test`
Expected: PASS

**Step 6: Commit**

```bash
git add src/data_models.h src/data_models.c tests/test_data_models.c
git commit -m "feat: add data structures for stations and connections"
```

---

### Task 2: Implement Persistence Layer

**Files:**
- Create: `src/persistence.h`
- Create: `src/persistence.c`
- Test: `tests/test_persistence.c`

**Step 1: Write failing test for saving connections**

Create `tests/test_persistence.c`:

```c
#include <check.h>
#include "../src/persistence.h"

START_TEST(test_save_and_load_connections) {
    SavedConnection conns[2];
    conns[0] = create_saved_connection("8503000", "Zürich HB", "8507000", "Bern");
    conns[1] = create_saved_connection("8507000", "Bern", "8508500", "Interlaken");

    save_connections(conns, 2);

    SavedConnection loaded[10];
    int count = load_connections(loaded);

    ck_assert_int_eq(count, 2);
    ck_assert_str_eq(loaded[0].departure_station_name, "Zürich HB");
    ck_assert_str_eq(loaded[1].arrival_station_name, "Interlaken");
}
END_TEST
```

**Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL with "persistence.h not found"

**Step 3: Define persistence interface**

Create `src/persistence.h`:

```c
#pragma once
#include "data_models.h"

#define PERSIST_KEY_CONNECTIONS 1
#define PERSIST_KEY_FAVORITES 2
#define PERSIST_KEY_NUM_CONNECTIONS 3
#define PERSIST_KEY_NUM_FAVORITES 4

// Save/load saved connections
void save_connections(SavedConnection *connections, int count);
int load_connections(SavedConnection *connections);

// Save/load favorite stations
void save_favorites(Station *stations, int count);
int load_favorites(Station *stations);

// Check if connection limit reached
bool is_connection_limit_reached(void);
```

**Step 4: Implement persistence functions**

Create `src/persistence.c`:

```c
#include "persistence.h"

void save_connections(SavedConnection *connections, int count) {
    if (count > MAX_SAVED_CONNECTIONS) {
        count = MAX_SAVED_CONNECTIONS;
    }
    persist_write_int(PERSIST_KEY_NUM_CONNECTIONS, count);
    persist_write_data(PERSIST_KEY_CONNECTIONS, connections,
                       sizeof(SavedConnection) * count);
}

int load_connections(SavedConnection *connections) {
    if (!persist_exists(PERSIST_KEY_NUM_CONNECTIONS)) {
        return 0;
    }
    int count = persist_read_int(PERSIST_KEY_NUM_CONNECTIONS);
    persist_read_data(PERSIST_KEY_CONNECTIONS, connections,
                      sizeof(SavedConnection) * count);
    return count;
}

void save_favorites(Station *stations, int count) {
    if (count > MAX_FAVORITE_STATIONS) {
        count = MAX_FAVORITE_STATIONS;
    }
    persist_write_int(PERSIST_KEY_NUM_FAVORITES, count);
    persist_write_data(PERSIST_KEY_FAVORITES, stations,
                       sizeof(Station) * count);
}

int load_favorites(Station *stations) {
    if (!persist_exists(PERSIST_KEY_NUM_FAVORITES)) {
        return 0;
    }
    int count = persist_read_int(PERSIST_KEY_NUM_FAVORITES);
    persist_read_data(PERSIST_KEY_FAVORITES, stations,
                      sizeof(Station) * count);
    return count;
}

bool is_connection_limit_reached(void) {
    if (!persist_exists(PERSIST_KEY_NUM_CONNECTIONS)) {
        return false;
    }
    int count = persist_read_int(PERSIST_KEY_NUM_CONNECTIONS);
    return count >= MAX_SAVED_CONNECTIONS;
}
```

**Step 5: Run test to verify it passes**

Run: `make test`
Expected: PASS

**Step 6: Commit**

```bash
git add src/persistence.h src/persistence.c tests/test_persistence.c
git commit -m "feat: add persistence layer for connections and favorites"
```

---

## Phase 2: Phone-Side JavaScript (PebbleKit JS)

### Task 3: SBB API Client

**Files:**
- Create: `src/pkjs/index.js`
- Create: `src/pkjs/sbb_api.js`

**Step 1: Create SBB API client module**

Create `src/pkjs/sbb_api.js`:

```javascript
// SBB OpenData API endpoint
const SBB_API_BASE = 'https://transport.opendata.ch/v1';

// Fetch nearby stations based on coordinates
function fetchNearbyStations(lat, lon, callback) {
    const url = `${SBB_API_BASE}/locations?x=${lon}&y=${lat}&type=station`;

    fetch(url)
        .then(response => response.json())
        .then(data => {
            const stations = data.stations.slice(0, 10).map(station => ({
                id: station.id,
                name: station.name,
                distance: Math.round(station.distance || 0)
            }));
            callback(null, stations);
        })
        .catch(error => {
            console.error('Error fetching nearby stations:', error);
            callback(error, null);
        });
}

// Fetch connections between two stations
function fetchConnections(fromId, toId, callback) {
    const url = `${SBB_API_BASE}/connections?from=${fromId}&to=${toId}&limit=5`;

    fetch(url)
        .then(response => response.json())
        .then(data => {
            const connections = data.connections.map(conn => {
                const sections = conn.sections.map(section => ({
                    departureStation: section.departure.station.name,
                    arrivalStation: section.arrival.station.name,
                    departureTime: Math.floor(new Date(section.departure.departure).getTime() / 1000),
                    arrivalTime: Math.floor(new Date(section.arrival.arrival).getTime() / 1000),
                    platform: section.departure.platform || 'N/A',
                    trainType: section.journey ? section.journey.category + ' ' + section.journey.number : 'Walk',
                    delayMinutes: section.departure.delay || 0
                }));

                return {
                    sections: sections,
                    numSections: sections.length,
                    departureTime: Math.floor(new Date(conn.from.departure).getTime() / 1000),
                    arrivalTime: Math.floor(new Date(conn.to.arrival).getTime() / 1000),
                    totalDelayMinutes: conn.from.delay || 0,
                    numChanges: sections.length - 1
                };
            });
            callback(null, connections);
        })
        .catch(error => {
            console.error('Error fetching connections:', error);
            callback(error, null);
        });
}

module.exports = {
    fetchNearbyStations,
    fetchConnections
};
```

**Step 2: Test API client manually**

Add test endpoint to verify:

```javascript
// Test in browser console or Node.js
fetchNearbyStations(47.3769, 8.5417, (err, stations) => {
    console.log('Stations:', stations);
});
```

Expected: Returns array of stations near Zürich

**Step 3: Commit**

```bash
git add src/pkjs/sbb_api.js
git commit -m "feat: add SBB API client for stations and connections"
```

---

### Task 4: GPS Location Service

**Files:**
- Create: `src/pkjs/location_service.js`
- Modify: `src/pkjs/index.js`

**Step 1: Create location service module**

Create `src/pkjs/location_service.js`:

```javascript
const sbbApi = require('./sbb_api');

let lastKnownStations = [];
let lastGPSTime = 0;
const GPS_CACHE_DURATION = 10 * 60 * 1000; // 10 minutes

function requestNearbyStations(callback) {
    const now = Date.now();

    // Return cached stations if recent
    if (lastKnownStations.length > 0 && (now - lastGPSTime) < GPS_CACHE_DURATION) {
        console.log('Using cached nearby stations');
        callback(null, lastKnownStations);
        return;
    }

    // Request GPS location
    navigator.geolocation.getCurrentPosition(
        position => {
            const lat = position.coords.latitude;
            const lon = position.coords.longitude;
            console.log(`GPS: ${lat}, ${lon}`);

            sbbApi.fetchNearbyStations(lat, lon, (err, stations) => {
                if (err) {
                    // If error but have cached data, return cached
                    if (lastKnownStations.length > 0) {
                        callback(null, lastKnownStations);
                    } else {
                        callback(err, null);
                    }
                } else {
                    lastKnownStations = stations;
                    lastGPSTime = now;
                    callback(null, stations);
                }
            });
        },
        error => {
            console.error('GPS error:', error);
            // Return cached stations if available
            if (lastKnownStations.length > 0) {
                callback(null, lastKnownStations);
            } else {
                callback(error, null);
            }
        },
        {
            timeout: 10000,
            maximumAge: GPS_CACHE_DURATION,
            enableHighAccuracy: false
        }
    );
}

module.exports = {
    requestNearbyStations
};
```

**Step 2: Commit**

```bash
git add src/pkjs/location_service.js
git commit -m "feat: add GPS location service with caching"
```

---

### Task 5: AppMessage Handler

**Files:**
- Create: `src/pkjs/message_handler.js`
- Modify: `src/pkjs/index.js`

**Step 1: Create message handler**

Create `src/pkjs/message_handler.js`:

```javascript
const sbbApi = require('./sbb_api');
const locationService = require('./location_service');

function handleAppMessage(event) {
    const message = event.payload;
    console.log('Received message:', JSON.stringify(message));

    if (message.REQUEST_NEARBY_STATIONS !== undefined) {
        handleNearbyStationsRequest();
    } else if (message.REQUEST_CONNECTIONS !== undefined) {
        handleConnectionsRequest(
            message.DEPARTURE_STATION_ID,
            message.ARRIVAL_STATION_ID
        );
    }
}

function handleNearbyStationsRequest() {
    locationService.requestNearbyStations((err, stations) => {
        if (err) {
            sendError('GPS unavailable. Check location settings.');
            return;
        }

        // Send stations one at a time to avoid message size limits
        stations.forEach((station, index) => {
            Pebble.sendAppMessage({
                STATION_ID: station.id,
                STATION_NAME: station.name,
                STATION_DISTANCE: station.distance
            }, () => {
                console.log(`Sent station ${index + 1}/${stations.length}`);
            }, () => {
                console.error(`Failed to send station ${station.name}`);
            });
        });
    });
}

function handleConnectionsRequest(fromId, toId) {
    if (!fromId || !toId) {
        sendError('Invalid station IDs');
        return;
    }

    sbbApi.fetchConnections(fromId, toId, (err, connections) => {
        if (err) {
            sendError('Network error. Check connection.');
            return;
        }

        if (connections.length === 0) {
            sendError('No connections found');
            return;
        }

        // Send first connection (we'll enhance this later for multiple)
        const conn = connections[0];
        Pebble.sendAppMessage({
            CONNECTION_DATA: 1,
            DEPARTURE_TIME: conn.departureTime,
            ARRIVAL_TIME: conn.arrivalTime,
            PLATFORM: conn.sections[0].platform,
            TRAIN_TYPE: conn.sections[0].trainType,
            DELAY_MINUTES: conn.totalDelayMinutes,
            NUM_CHANGES: conn.numChanges
        }, () => {
            console.log('Sent connection data');
        }, () => {
            console.error('Failed to send connection data');
        });
    });
}

function sendError(message) {
    Pebble.sendAppMessage({
        ERROR_MESSAGE: message
    }, () => {
        console.log('Sent error:', message);
    }, () => {
        console.error('Failed to send error message');
    });
}

module.exports = {
    handleAppMessage
};
```

**Step 2: Wire up in main JS file**

Create `src/pkjs/index.js`:

```javascript
const messageHandler = require('./message_handler');

Pebble.addEventListener('ready', event => {
    console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', event => {
    messageHandler.handleAppMessage(event);
});
```

**Step 3: Commit**

```bash
git add src/pkjs/message_handler.js src/pkjs/index.js
git commit -m "feat: add AppMessage handler for watch communication"
```

---

## Phase 3: Watch-Side UI (C)

### Task 6: Main Menu Window

**Files:**
- Create: `src/main_window.h`
- Create: `src/main_window.c`
- Modify: `src/main.c`

**Step 1: Define main window interface**

Create `src/main_window.h`:

```c
#pragma once
#include <pebble.h>

void main_window_push(void);
void main_window_pop(void);
void main_window_refresh(void);
```

**Step 2: Implement main window**

Create `src/main_window.c`:

```c
#include "main_window.h"
#include "data_models.h"
#include "persistence.h"

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
        // TODO: Push add connection window
        return;
    }
    // TODO: Push connection detail window
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
```

**Step 3: Update main.c to use main window**

Modify `src/main.c`:

```c
#include <pebble.h>
#include "main_window.h"

static void init(void) {
    main_window_push();
}

static void deinit(void) {
    main_window_pop();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
```

**Step 4: Build and test in emulator**

```bash
pebble build
pebble install --emulator aplite
```

Expected: Main window displays with "Add Connection" prompt

**Step 5: Commit**

```bash
git add src/main_window.h src/main_window.c src/main.c
git commit -m "feat: add main menu window with connection list"
```

---

### Task 7: Station Selection Window

**Files:**
- Create: `src/station_select_window.h`
- Create: `src/station_select_window.c`

**Step 1: Define station selection interface**

Create `src/station_select_window.h`:

```c
#pragma once
#include <pebble.h>
#include "data_models.h"

typedef void (*StationSelectCallback)(Station *station);

void station_select_window_push(StationSelectCallback callback);
void station_select_window_add_station(Station station);
void station_select_window_clear_stations(void);
```

**Step 2: Implement station selection window**

Create `src/station_select_window.c`:

```c
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
```

**Step 3: Build and test**

```bash
pebble build
pebble install --emulator aplite
```

Expected: Station selection window displays with loading message

**Step 4: Commit**

```bash
git add src/station_select_window.h src/station_select_window.c
git commit -m "feat: add station selection window with GPS search"
```

---

### Task 8: Add Connection Flow

**Files:**
- Create: `src/add_connection_window.h`
- Create: `src/add_connection_window.c`
- Modify: `src/main_window.c`

**Step 1: Define add connection interface**

Create `src/add_connection_window.h`:

```c
#pragma once
#include <pebble.h>

void add_connection_window_push(void);
```

**Step 2: Implement add connection window**

Create `src/add_connection_window.c`:

```c
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
    text_layer_set_text(s_instruction_layer, "Select arrival station");

    // Push station select for arrival
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
```

**Step 3: Wire up in main window**

Modify `src/main_window.c` - add to includes:

```c
#include "add_connection_window.h"
```

Update `menu_select_callback`:

```c
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (s_num_connections == 0) {
        add_connection_window_push();
        return;
    }
    // TODO: Push connection detail window
}
```

**Step 4: Build and test**

```bash
pebble build
pebble install --emulator aplite
```

Expected: Can navigate through add connection flow

**Step 5: Commit**

```bash
git add src/add_connection_window.h src/add_connection_window.c src/main_window.c
git commit -m "feat: add connection creation flow with two-step wizard"
```

---

## Phase 4: Connection Display and Real-time Updates

### Task 9: Connection Detail Window

**Files:**
- Create: `src/connection_detail_window.h`
- Create: `src/connection_detail_window.c`
- Modify: `src/main_window.c`

**Step 1: Define connection detail interface**

Create `src/connection_detail_window.h`:

```c
#pragma once
#include <pebble.h>
#include "data_models.h"

void connection_detail_window_push(SavedConnection *connection);
void connection_detail_window_update_data(Connection *connections, int count);
```

**Step 2: Implement connection detail window**

Create `src/connection_detail_window.c`:

```c
#include "connection_detail_window.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static SavedConnection s_connection;
static Connection s_connections[5];
static int s_num_connections = 0;
static TextLayer *s_status_layer;
static AppTimer *s_refresh_timer;

#define REFRESH_INTERVAL_MS 60000  // 60 seconds

static void request_connections(void);

static void format_time(time_t timestamp, char *buffer, size_t size) {
    struct tm *tm_info = localtime(&timestamp);
    strftime(buffer, size, "%H:%M", tm_info);
}

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
    static char header[64];
    snprintf(header, sizeof(header), "%s → %s",
             s_connection.departure_station_name,
             s_connection.arrival_station_name);
    menu_cell_basic_header_draw(ctx, cell_layer, header);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    if (s_num_connections == 0) {
        menu_cell_basic_draw(ctx, cell_layer, "Loading...", "Fetching trains", NULL);
        return;
    }

    Connection *conn = &s_connections[cell_index->row];
    static char title[64];
    static char subtitle[64];

    char dep_time[6], arr_time[6];
    format_time(conn->departure_time, dep_time, sizeof(dep_time));
    format_time(conn->arrival_time, arr_time, sizeof(arr_time));

    // Format: "14:32 → 15:47 | 1 chg | Pl.7"
    if (conn->num_changes > 0) {
        snprintf(title, sizeof(title), "%s → %s | %d chg",
                 dep_time, arr_time, conn->num_changes);
    } else {
        snprintf(title, sizeof(title), "%s → %s | Direct",
                 dep_time, arr_time);
    }

    // Format subtitle with delay info
    if (conn->sections[0].platform[0] != '\0') {
        if (conn->total_delay_minutes > 0) {
            snprintf(subtitle, sizeof(subtitle), "Pl.%s | %s | +%d min",
                     conn->sections[0].platform,
                     conn->sections[0].train_type,
                     conn->total_delay_minutes);
        } else {
            snprintf(subtitle, sizeof(subtitle), "Pl.%s | %s | ✓",
                     conn->sections[0].platform,
                     conn->sections[0].train_type);
        }
    }

    menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void refresh_timer_callback(void *data) {
    request_connections();
    s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void request_connections(void) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_CONNECTIONS, 1);
    dict_write_cstring(iter, MESSAGE_KEY_DEPARTURE_STATION_ID, s_connection.departure_station_id);
    dict_write_cstring(iter, MESSAGE_KEY_ARRIVAL_STATION_ID, s_connection.arrival_station_id);
    app_message_outbox_send();
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Status layer
    s_status_layer = text_layer_create(GRect(0, bounds.size.h - 20, bounds.size.w, 20));
    text_layer_set_text(s_status_layer, "Updating...");
    text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
    text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(s_status_layer));

    // Menu layer
    GRect menu_bounds = GRect(0, 0, bounds.size.w, bounds.size.h - 20);
    s_menu_layer = menu_layer_create(menu_bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    // Request initial data
    request_connections();

    // Start refresh timer
    s_refresh_timer = app_timer_register(REFRESH_INTERVAL_MS, refresh_timer_callback, NULL);
}

static void window_unload(Window *window) {
    if (s_refresh_timer) {
        app_timer_cancel(s_refresh_timer);
        s_refresh_timer = NULL;
    }
    text_layer_destroy(s_status_layer);
    menu_layer_destroy(s_menu_layer);
}

void connection_detail_window_push(SavedConnection *connection) {
    s_connection = *connection;
    s_num_connections = 0;

    if (!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}

void connection_detail_window_update_data(Connection *connections, int count) {
    s_num_connections = count;
    if (count > 5) s_num_connections = 5;

    for (int i = 0; i < s_num_connections; i++) {
        s_connections[i] = connections[i];
    }

    menu_layer_reload_data(s_menu_layer);
    text_layer_set_text(s_status_layer, "Updated");
}
```

**Step 3: Update main window to open detail view**

Modify `src/main_window.c` - add include:

```c
#include "connection_detail_window.h"
```

Update `menu_select_callback`:

```c
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (s_num_connections == 0) {
        add_connection_window_push();
        return;
    }
    SavedConnection *conn = &s_connections[cell_index->row];
    connection_detail_window_push(conn);
}
```

**Step 4: Build and test**

```bash
pebble build
pebble install --emulator aplite
```

Expected: Can view connection details with auto-refresh

**Step 5: Commit**

```bash
git add src/connection_detail_window.h src/connection_detail_window.c src/main_window.c
git commit -m "feat: add connection detail view with auto-refresh"
```

---

## Phase 5: AppMessage Integration

### Task 10: Complete AppMessage Handler in C

**Files:**
- Create: `src/app_message.h`
- Create: `src/app_message.c`
- Modify: `src/main.c`

**Step 1: Define AppMessage interface**

Create `src/app_message.h`:

```c
#pragma once
#include <pebble.h>

void app_message_init(void);
void app_message_deinit(void);
```

**Step 2: Implement AppMessage handlers**

Create `src/app_message.c`:

```c
#include "app_message.h"
#include "station_select_window.h"
#include "connection_detail_window.h"
#include "data_models.h"

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Check for station data
    Tuple *station_name_tuple = dict_find(iterator, MESSAGE_KEY_STATION_NAME);
    Tuple *station_id_tuple = dict_find(iterator, MESSAGE_KEY_STATION_ID);
    Tuple *station_distance_tuple = dict_find(iterator, MESSAGE_KEY_STATION_DISTANCE);

    if (station_name_tuple && station_id_tuple && station_distance_tuple) {
        Station station = create_station(
            station_id_tuple->value->cstring,
            station_name_tuple->value->cstring,
            station_distance_tuple->value->int32
        );
        station_select_window_add_station(station);
        return;
    }

    // Check for connection data
    Tuple *conn_data_tuple = dict_find(iterator, MESSAGE_KEY_CONNECTION_DATA);
    if (conn_data_tuple) {
        Connection conn;
        conn.num_sections = 1;  // Simple case for now

        Tuple *dep_time = dict_find(iterator, MESSAGE_KEY_DEPARTURE_TIME);
        Tuple *arr_time = dict_find(iterator, MESSAGE_KEY_ARRIVAL_TIME);
        Tuple *platform = dict_find(iterator, MESSAGE_KEY_PLATFORM);
        Tuple *train_type = dict_find(iterator, MESSAGE_KEY_TRAIN_TYPE);
        Tuple *delay = dict_find(iterator, MESSAGE_KEY_DELAY_MINUTES);
        Tuple *num_changes = dict_find(iterator, MESSAGE_KEY_NUM_CHANGES);

        if (dep_time && arr_time) {
            conn.departure_time = dep_time->value->int32;
            conn.arrival_time = arr_time->value->int32;
            conn.total_delay_minutes = delay ? delay->value->int8 : 0;
            conn.num_changes = num_changes ? num_changes->value->int8 : 0;

            // First section
            conn.sections[0].departure_time = conn.departure_time;
            conn.sections[0].arrival_time = conn.arrival_time;
            conn.sections[0].delay_minutes = conn.total_delay_minutes;

            if (platform) {
                strncpy(conn.sections[0].platform, platform->value->cstring, MAX_PLATFORM_LENGTH - 1);
            }
            if (train_type) {
                strncpy(conn.sections[0].train_type, train_type->value->cstring, MAX_TRAIN_TYPE_LENGTH - 1);
            }

            connection_detail_window_update_data(&conn, 1);
        }
        return;
    }

    // Check for error
    Tuple *error_tuple = dict_find(iterator, MESSAGE_KEY_ERROR_MESSAGE);
    if (error_tuple) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error from JS: %s", error_tuple->value->cstring);
        // TODO: Show error dialog
        return;
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox sent successfully");
}

void app_message_init(void) {
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    app_message_open(512, 512);
}

void app_message_deinit(void) {
    app_message_deregister_callbacks();
}
```

**Step 3: Initialize in main**

Modify `src/main.c`:

```c
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
```

**Step 4: Build and test end-to-end**

```bash
pebble build
pebble install --emulator aplite
```

Expected: Full communication between watch and phone

**Step 5: Commit**

```bash
git add src/app_message.h src/app_message.c src/main.c
git commit -m "feat: integrate AppMessage for watch-phone communication"
```

---

## Phase 6: Testing and Polish

### Task 11: Emulator Testing

**Step 1: Test basic flow in emulator**

```bash
pebble build
pebble install --emulator aplite
```

Test checklist:
- [ ] App launches and shows main menu
- [ ] Can start add connection flow
- [ ] Station selection shows loading state
- [ ] Can save a connection
- [ ] Connection appears in main menu
- [ ] Can view connection details
- [ ] Loading state shows in connection details

**Step 2: Test with mock data**

Temporarily modify JS to return mock data for testing without GPS.

**Step 3: Document any bugs found**

Create `docs/testing-notes.md` with findings.

**Step 4: Commit testing notes**

```bash
git add docs/testing-notes.md
git commit -m "docs: add emulator testing notes"
```

---

### Task 12: Error Handling and Edge Cases

**Files:**
- Create: `src/error_dialog.h`
- Create: `src/error_dialog.c`

**Step 1: Create error dialog utility**

Create `src/error_dialog.h`:

```c
#pragma once
#include <pebble.h>

void show_error_dialog(const char *title, const char *message);
```

Create `src/error_dialog.c`:

```c
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
```

**Step 2: Integrate error dialogs**

Add error handling to `src/app_message.c`:

```c
#include "error_dialog.h"

// In inbox_received_callback, update error handling:
Tuple *error_tuple = dict_find(iterator, MESSAGE_KEY_ERROR_MESSAGE);
if (error_tuple) {
    show_error_dialog("Error", error_tuple->value->cstring);
    return;
}
```

**Step 3: Build and test**

```bash
pebble build
pebble install --emulator aplite
```

Expected: Error dialogs appear for network errors

**Step 4: Commit**

```bash
git add src/error_dialog.h src/error_dialog.c src/app_message.c
git commit -m "feat: add error dialog for user feedback"
```

---

## Phase 7: Documentation and Final Polish

### Task 13: Add README

**Files:**
- Create: `README.md`

**Step 1: Write comprehensive README**

Create `README.md`:

```markdown
# SBB Train Schedule for Pebble

A Pebble smartwatch app for viewing Swiss train schedules with real-time delay information.

## Features

- Save favorite train connections (e.g., "Home → Work")
- GPS-based station selection
- Real-time delay information from SBB OpenData API
- Multi-leg journey support with transfer information
- Auto-refreshing schedule updates every 60 seconds
- Works on Pebble 2 (black & white display)

## Installation

### Prerequisites

- Python 3.x
- Pebble SDK 4.x

### Setup

```bash
# Install Pebble SDK
pip install pebble-tool
pebble sdk install

# Build the app
pebble build

# Install on emulator
pebble install --emulator aplite

# Install on physical watch
pebble install --phone YOUR_PHONE_IP
```

## Usage

1. Launch app to see saved connections
2. Long-press UP button to add a new connection
3. Select departure station (GPS finds nearby stations)
4. Select arrival station
5. View upcoming trains with real-time delays
6. Connection refreshes automatically every 60 seconds

## Development

### Project Structure

```
src/
├── main.c                      # App entry point
├── data_models.h/c             # Data structures
├── persistence.h/c             # Local storage
├── app_message.h/c             # Watch-phone communication
├── main_window.h/c             # Main menu
├── connection_detail_window.h/c # Train schedule view
├── station_select_window.h/c   # Station picker
├── add_connection_window.h/c   # Add connection wizard
├── error_dialog.h/c            # Error display
└── pkjs/
    ├── index.js                # PebbleKit JS entry
    ├── sbb_api.js              # SBB API client
    ├── location_service.js     # GPS handler
    └── message_handler.js      # Message routing
```

### Testing

```bash
# Run in emulator
pebble build && pebble install --emulator aplite

# View logs
pebble logs
```

### Architecture

- **Watch (C)**: UI, persistence, user input
- **Phone (JavaScript)**: API calls, GPS, data processing
- **Communication**: Pebble AppMessage protocol

## API

Uses [SBB OpenData Transport API](https://transport.opendata.ch/)

## License

MIT

## Credits

Built with Pebble SDK and SBB OpenData API
```

**Step 2: Commit**

```bash
git add README.md
git commit -m "docs: add comprehensive README"
```

---

### Task 14: Final Build and Verification

**Step 1: Clean build**

```bash
pebble clean
pebble build
```

Expected: BUILD SUCCESSFUL with no warnings

**Step 2: Verify in emulator**

```bash
pebble install --emulator aplite
```

Test complete user flow:
1. Launch app
2. Add connection
3. View trains
4. Check auto-refresh

**Step 3: Create release notes**

Create `docs/RELEASE-NOTES.md`:

```markdown
# Release Notes

## v1.0 (2025-11-07)

Initial release of SBB Train Schedule for Pebble.

### Features
- Save up to 10 favorite train connections
- GPS-based station selection with 20 favorite stations
- Real-time delay information
- Multi-leg journey support
- Auto-refresh every 60 seconds
- Black & white display optimized for Pebble 2

### Known Limitations
- Maximum 10 saved connections
- Requires phone GPS for station discovery
- Network required for real-time updates

### Technical
- Pebble SDK 4.x
- SBB OpenData API integration
- PebbleKit JS for networking
```

**Step 4: Final commit**

```bash
git add docs/RELEASE-NOTES.md
git commit -m "docs: add v1.0 release notes"
```

**Step 5: Create release tag**

```bash
git tag -a v1.0 -m "Release version 1.0 - Initial SBB Train Schedule app"
```

---

## Completion Checklist

- [ ] All data models implemented and tested
- [ ] Persistence layer working
- [ ] SBB API integration complete
- [ ] GPS location service functional
- [ ] All UI windows implemented
- [ ] AppMessage communication working
- [ ] Error handling in place
- [ ] Emulator testing passed
- [ ] Documentation complete
- [ ] README written
- [ ] Release notes created
- [ ] Git tags created

## Next Steps (Future Enhancements)

1. Add voice input for station selection
2. Implement push notifications for delays
3. Integrate with Pebble Timeline
4. Support other Swiss transport (trams, buses)
5. Add journey planning from current location
6. Implement favorites management UI
7. Add multi-connection display (show all 5 trains)
8. Improve multi-leg journey detail view
9. Add connection deletion feature
10. Optimize battery usage

---

**Plan Status:** Complete and ready for execution with @superpowers:executing-plans or @superpowers:subagent-driven-development
