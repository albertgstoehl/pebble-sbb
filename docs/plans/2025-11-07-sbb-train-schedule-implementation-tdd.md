# SBB Train Schedule Pebble App Implementation Plan (TDD)

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for EVERY task. RED-GREEN-REFACTOR is mandatory.

**Goal:** Build a Pebble watch app that displays Swiss train schedules with saved connections, GPS-based station selection, and real-time delay information.

**Architecture:** PebbleKit JS handles networking (SBB OpenData API) and GPS on phone; C handles UI rendering, persistence, and user interaction on watch; AppMessage protocol for communication.

**Tech Stack:** C (Pebble SDK), JavaScript (PebbleKit JS), SBB OpenData API, Pebble SDK 4.x, QEMU emulator (aplite platform), Jest (JS testing)

**Testing Strategy:** TDD throughout - write failing test first, watch it fail, write minimal code to pass, refactor.

---

## Prerequisites

### Task 0: Development Environment Setup

**Files:**
- Create: `package.json`
- Create: `appinfo.json`
- Create: `wscript`
- Create: `.gitignore`

**Step 1: Check for existing Pebble SDK**

```bash
pebble --version
```

Expected: Version displayed OR command not found

**Step 2: Install Pebble SDK if needed**

```bash
pip install pebble-tool
pebble sdk install
```

Expected: SDK installed successfully, `pebble` command available

**Step 3: Create minimal project structure**

```bash
mkdir -p src src/pkjs resources tests
```

**Step 4: Create package.json for JavaScript dependencies**

Create `package.json`:

```json
{
  "name": "sbb-train-schedule",
  "version": "1.0.0",
  "description": "SBB train schedule app for Pebble",
  "main": "src/pkjs/index.js",
  "scripts": {
    "test": "jest",
    "test:watch": "jest --watch"
  },
  "devDependencies": {
    "jest": "^29.0.0",
    "@types/jest": "^29.0.0"
  },
  "jest": {
    "testEnvironment": "node",
    "testMatch": ["**/tests/**/*.test.js"]
  }
}
```

**Step 5: Install Jest for JavaScript testing**

```bash
npm install
```

Expected: jest installed successfully

**Step 6: Configure appinfo.json**

Create `appinfo.json`:

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

**Step 7: Create .gitignore**

Create `.gitignore`:

```
build/
node_modules/
*.pbw
.lock-waf_*
```

**Step 8: Create minimal main.c to verify build**

Create `src/main.c`:

```c
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
```

**Step 9: Create wscript for build configuration**

Create `wscript`:

```python
top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')
    ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'), target='pebble-app.elf')
    ctx.pbl_bundle(elf='pebble-app.elf', js=ctx.path.ant_glob('src/pkjs/**/*.js'))
```

**Step 10: Verify build**

```bash
pebble build
```

Expected: BUILD SUCCESSFUL

**Step 11: Commit initial setup**

```bash
git add .
git commit -m "feat: initialize Pebble project with TDD setup

- Pebble SDK configuration
- Jest for JavaScript testing
- Basic project structure"
```

---

## Phase 1: Data Models and Persistence (C)

### Task 1: Define Data Structures

**Files:**
- Create: `src/data_models.h`
- Create: `src/data_models.c`
- Create: `tests/test_data_models.c`
- Create: `tests/Makefile`

**RED - Step 1: Write failing test for SavedConnection**

Create `tests/test_data_models.c`:

```c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/data_models.h"

void test_create_saved_connection(void) {
    SavedConnection conn = create_saved_connection(
        "8503000", "Zürich HB",
        "8507000", "Bern"
    );

    assert(strcmp(conn.departure_station_id, "8503000") == 0);
    assert(strcmp(conn.departure_station_name, "Zürich HB") == 0);
    assert(strcmp(conn.arrival_station_id, "8507000") == 0);
    assert(strcmp(conn.arrival_station_name, "Bern") == 0);

    printf("test_create_saved_connection: PASS\n");
}

void test_create_station(void) {
    Station station = create_station("8503000", "Zürich HB", 1200);

    assert(strcmp(station.id, "8503000") == 0);
    assert(strcmp(station.name, "Zürich HB") == 0);
    assert(station.distance_meters == 1200);

    printf("test_create_station: PASS\n");
}

void test_truncate_long_station_name(void) {
    char long_name[100];
    memset(long_name, 'A', 99);
    long_name[99] = '\0';

    Station station = create_station("123", long_name, 100);

    // Should truncate to MAX_STATION_NAME_LENGTH - 1
    assert(strlen(station.name) < 100);
    assert(station.name[MAX_STATION_NAME_LENGTH - 1] == '\0');

    printf("test_truncate_long_station_name: PASS\n");
}

int main(void) {
    test_create_saved_connection();
    test_create_station();
    test_truncate_long_station_name();
    printf("\nAll data_models tests passed!\n");
    return 0;
}
```

Create `tests/Makefile`:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g -I../src

# Mock pebble.h for testing
PEBBLE_MOCK = -include test_pebble_mock.h

test_data_models: test_data_models.c ../src/data_models.c
	$(CC) $(CFLAGS) $(PEBBLE_MOCK) -o $@ $^

clean:
	rm -f test_data_models test_persistence

.PHONY: clean
```

Create `tests/test_pebble_mock.h`:

```c
#ifndef TEST_PEBBLE_MOCK_H
#define TEST_PEBBLE_MOCK_H

// Mock Pebble SDK types for testing
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Add other mocks as needed

#endif
```

**Verify RED - Step 2: Run test to confirm it fails**

```bash
cd tests
make test_data_models
./test_data_models
```

Expected: Compilation fails with "data_models.h: No such file or directory"

**GREEN - Step 3: Define data structures in header**

Create `src/data_models.h`:

```c
#pragma once
#ifndef PEBBLE_H
#include <pebble.h>
#endif

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

**GREEN - Step 4: Implement minimal functions**

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

**Verify GREEN - Step 5: Run test to confirm it passes**

```bash
cd tests
make clean
make test_data_models
./test_data_models
```

Expected: All tests PASS

**REFACTOR - Step 6: Clean up if needed**

(No refactoring needed - code is minimal)

**Step 7: Commit**

```bash
git add src/data_models.h src/data_models.c tests/
git commit -m "feat: add data structures for stations and connections

TDD: All tests passing
- SavedConnection creation
- Station creation
- Long name truncation"
```

---

### Task 2: Implement Persistence Layer

**Files:**
- Create: `src/persistence.h`
- Create: `src/persistence.c`
- Create: `tests/test_persistence.c`

**RED - Step 1: Write failing test for save/load connections**

Create `tests/test_persistence.c`:

```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/persistence.h"

// Mock persist functions for testing
#define PERSIST_DATA_MAX_LENGTH 4096
static uint8_t persist_storage[10][PERSIST_DATA_MAX_LENGTH];
static bool persist_exists_flags[10];

bool persist_exists(uint32_t key) {
    return persist_exists_flags[key];
}

int persist_write_data(uint32_t key, const void *data, size_t size) {
    memcpy(persist_storage[key], data, size);
    persist_exists_flags[key] = true;
    return size;
}

int persist_read_data(uint32_t key, void *buffer, size_t size) {
    memcpy(buffer, persist_storage[key], size);
    return size;
}

int persist_write_int(uint32_t key, int value) {
    memcpy(persist_storage[key], &value, sizeof(int));
    persist_exists_flags[key] = true;
    return sizeof(int);
}

int persist_read_int(uint32_t key) {
    int value;
    memcpy(&value, persist_storage[key], sizeof(int));
    return value;
}

void test_save_and_load_connections(void) {
    SavedConnection conns[2];
    conns[0] = create_saved_connection("8503000", "Zürich HB", "8507000", "Bern");
    conns[1] = create_saved_connection("8507000", "Bern", "8508500", "Interlaken");

    save_connections(conns, 2);

    SavedConnection loaded[MAX_SAVED_CONNECTIONS];
    int count = load_connections(loaded);

    assert(count == 2);
    assert(strcmp(loaded[0].departure_station_name, "Zürich HB") == 0);
    assert(strcmp(loaded[1].arrival_station_name, "Interlaken") == 0);

    printf("test_save_and_load_connections: PASS\n");
}

void test_load_when_empty(void) {
    // Clear storage
    memset(persist_exists_flags, 0, sizeof(persist_exists_flags));

    SavedConnection loaded[MAX_SAVED_CONNECTIONS];
    int count = load_connections(loaded);

    assert(count == 0);

    printf("test_load_when_empty: PASS\n");
}

void test_connection_limit_reached(void) {
    SavedConnection conns[MAX_SAVED_CONNECTIONS];
    for (int i = 0; i < MAX_SAVED_CONNECTIONS; i++) {
        conns[i] = create_saved_connection("123", "A", "456", "B");
    }
    save_connections(conns, MAX_SAVED_CONNECTIONS);

    assert(is_connection_limit_reached() == true);

    printf("test_connection_limit_reached: PASS\n");
}

int main(void) {
    test_save_and_load_connections();
    test_load_when_empty();
    test_connection_limit_reached();
    printf("\nAll persistence tests passed!\n");
    return 0;
}
```

Update `tests/Makefile`:

```makefile
test_persistence: test_persistence.c ../src/persistence.c ../src/data_models.c
	$(CC) $(CFLAGS) $(PEBBLE_MOCK) -o $@ $^

all: test_data_models test_persistence

.PHONY: all clean
```

**Verify RED - Step 2: Run test to verify it fails**

```bash
cd tests
make test_persistence
```

Expected: FAIL with "persistence.h: No such file or directory"

**GREEN - Step 3: Define persistence interface**

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

**GREEN - Step 4: Implement persistence functions**

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

**Verify GREEN - Step 5: Run test to verify it passes**

```bash
cd tests
make clean
make test_persistence
./test_persistence
```

Expected: All tests PASS

**REFACTOR - Step 6: Clean up (if needed)**

(No refactoring needed - code is minimal)

**Step 7: Commit**

```bash
git add src/persistence.h src/persistence.c tests/test_persistence.c tests/Makefile
git commit -m "feat: add persistence layer for connections and favorites

TDD: All tests passing
- Save/load connections
- Save/load favorites
- Connection limit check
- Handle empty storage"
```

---

## Phase 2: Phone-Side JavaScript (PebbleKit JS) - TDD with Jest

### Task 3: SBB API Client

**Files:**
- Create: `src/pkjs/sbb_api.js`
- Create: `tests/sbb_api.test.js`

**RED - Step 1: Write failing test for fetchNearbyStations**

Create `tests/sbb_api.test.js`:

```javascript
const sbbApi = require('../src/pkjs/sbb_api');

// Mock global fetch
global.fetch = jest.fn();

describe('SBB API Client', () => {
  beforeEach(() => {
    fetch.mockClear();
  });

  test('fetchNearbyStations returns stations sorted by distance', async () => {
    const mockResponse = {
      stations: [
        { id: '8503000', name: 'Zürich HB', distance: 1200 },
        { id: '8503006', name: 'Zürich Stadelhofen', distance: 800 },
        { id: '8503020', name: 'Zürich Hardbrücke', distance: 2000 }
      ]
    };

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse
    });

    const result = await sbbApi.fetchNearbyStations(47.3769, 8.5417);

    expect(result).toHaveLength(3);
    expect(result[0].id).toBe('8503000');
    expect(result[0].name).toBe('Zürich HB');
    expect(result[0].distance).toBe(1200);
    expect(fetch).toHaveBeenCalledWith(
      expect.stringContaining('x=8.5417&y=47.3769')
    );
  });

  test('fetchNearbyStations limits to 10 stations', async () => {
    const mockStations = Array(20).fill(null).map((_, i) => ({
      id: `${8503000 + i}`,
      name: `Station ${i}`,
      distance: i * 100
    }));

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ stations: mockStations })
    });

    const result = await sbbApi.fetchNearbyStations(47.3769, 8.5417);

    expect(result).toHaveLength(10);
  });

  test('fetchNearbyStations handles fetch error', async () => {
    fetch.mockRejectedValueOnce(new Error('Network error'));

    await expect(
      sbbApi.fetchNearbyStations(47.3769, 8.5417)
    ).rejects.toThrow('Network error');
  });

  test('fetchConnections returns connection array', async () => {
    const mockResponse = {
      connections: [
        {
          from: {
            departure: '2025-11-07T14:32:00+0100',
            delay: 0
          },
          to: {
            arrival: '2025-11-07T15:47:00+0100'
          },
          sections: [
            {
              departure: {
                station: { name: 'Zürich HB' },
                departure: '2025-11-07T14:32:00+0100',
                platform: '7',
                delay: 0
              },
              arrival: {
                station: { name: 'Bern' },
                arrival: '2025-11-07T15:47:00+0100'
              },
              journey: {
                category: 'IC',
                number: '712'
              }
            }
          ]
        }
      ]
    };

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse
    });

    const result = await sbbApi.fetchConnections('8503000', '8507000');

    expect(result).toHaveLength(1);
    expect(result[0].sections).toHaveLength(1);
    expect(result[0].sections[0].trainType).toBe('IC 712');
    expect(result[0].sections[0].platform).toBe('7');
    expect(result[0].numChanges).toBe(0);
  });

  test('fetchConnections calculates number of changes correctly', async () => {
    const mockResponse = {
      connections: [
        {
          from: { departure: '2025-11-07T14:32:00+0100', delay: 0 },
          to: { arrival: '2025-11-07T16:02:00+0100' },
          sections: [
            { /* section 1 */ },
            { /* section 2 */ },
            { /* section 3 */ }
          ]
        }
      ]
    };

    // Mock full sections data
    mockResponse.connections[0].sections = mockResponse.connections[0].sections.map((_, i) => ({
      departure: {
        station: { name: `Station ${i}` },
        departure: '2025-11-07T14:32:00+0100',
        platform: '1',
        delay: 0
      },
      arrival: {
        station: { name: `Station ${i + 1}` },
        arrival: '2025-11-07T15:00:00+0100'
      },
      journey: { category: 'IC', number: '1' }
    }));

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse
    });

    const result = await sbbApi.fetchConnections('8503000', '8508500');

    expect(result[0].numChanges).toBe(2); // 3 sections = 2 changes
  });
});
```

**Verify RED - Step 2: Run test to verify it fails**

```bash
npm test sbb_api.test.js
```

Expected: FAIL with "Cannot find module '../src/pkjs/sbb_api'"

**GREEN - Step 3: Create minimal implementation**

Create `src/pkjs/sbb_api.js`:

```javascript
// SBB OpenData API endpoint
const SBB_API_BASE = 'https://transport.opendata.ch/v1';

// Fetch nearby stations based on coordinates
async function fetchNearbyStations(lat, lon) {
  const url = `${SBB_API_BASE}/locations?x=${lon}&y=${lat}&type=station`;

  const response = await fetch(url);
  const data = await response.json();

  const stations = data.stations.slice(0, 10).map(station => ({
    id: station.id,
    name: station.name,
    distance: Math.round(station.distance || 0)
  }));

  return stations;
}

// Fetch connections between two stations
async function fetchConnections(fromId, toId) {
  const url = `${SBB_API_BASE}/connections?from=${fromId}&to=${toId}&limit=5`;

  const response = await fetch(url);
  const data = await response.json();

  const connections = data.connections.map(conn => {
    const sections = conn.sections.map(section => ({
      departureStation: section.departure.station.name,
      arrivalStation: section.arrival.station.name,
      departureTime: Math.floor(new Date(section.departure.departure).getTime() / 1000),
      arrivalTime: Math.floor(new Date(section.arrival.arrival).getTime() / 1000),
      platform: section.departure.platform || 'N/A',
      trainType: section.journey ? `${section.journey.category} ${section.journey.number}` : 'Walk',
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

  return connections;
}

module.exports = {
  fetchNearbyStations,
  fetchConnections
};
```

**Verify GREEN - Step 4: Run test to verify it passes**

```bash
npm test sbb_api.test.js
```

Expected: All tests PASS

**REFACTOR - Step 5: Clean up if needed**

(Code is minimal and clear, no refactoring needed)

**Step 6: Commit**

```bash
git add src/pkjs/sbb_api.js tests/sbb_api.test.js
git commit -m "feat: add SBB API client with comprehensive tests

TDD: All tests passing
- Fetch nearby stations
- Limit to 10 stations
- Fetch connections
- Calculate transfer count
- Handle errors"
```

---

### Task 4: GPS Location Service

**Files:**
- Create: `src/pkjs/location_service.js`
- Create: `tests/location_service.test.js`

**RED - Step 1: Write failing test for GPS with caching**

Create `tests/location_service.test.js`:

```javascript
const locationService = require('../src/pkjs/location_service');

// Mock navigator.geolocation
global.navigator = {
  geolocation: {
    getCurrentPosition: jest.fn()
  }
};

// Mock sbb_api
jest.mock('../src/pkjs/sbb_api', () => ({
  fetchNearbyStations: jest.fn()
}));

const sbbApi = require('../src/pkjs/sbb_api');

describe('Location Service', () => {
  beforeEach(() => {
    navigator.geolocation.getCurrentPosition.mockClear();
    sbbApi.fetchNearbyStations.mockClear();
    locationService._clearCache(); // Helper to clear cache for testing
  });

  test('requestNearbyStations fetches GPS and returns stations', async () => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    navigator.geolocation.getCurrentPosition.mockImplementation((success) => {
      success(mockPosition);
    });

    sbbApi.fetchNearbyStations.mockResolvedValue(mockStations);

    const result = await locationService.requestNearbyStations();

    expect(result).toEqual(mockStations);
    expect(navigator.geolocation.getCurrentPosition).toHaveBeenCalled();
    expect(sbbApi.fetchNearbyStations).toHaveBeenCalledWith(47.3769, 8.5417);
  });

  test('requestNearbyStations uses cached data within 10 minutes', async () => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    navigator.geolocation.getCurrentPosition.mockImplementation((success) => {
      success(mockPosition);
    });

    sbbApi.fetchNearbyStations.mockResolvedValue(mockStations);

    // First call
    await locationService.requestNearbyStations();

    // Second call immediately after
    const result = await locationService.requestNearbyStations();

    expect(result).toEqual(mockStations);
    expect(navigator.geolocation.getCurrentPosition).toHaveBeenCalledTimes(1);
    expect(sbbApi.fetchNearbyStations).toHaveBeenCalledTimes(1);
  });

  test('requestNearbyStations re-fetches after cache expires', async () => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    navigator.geolocation.getCurrentPosition.mockImplementation((success) => {
      success(mockPosition);
    });

    sbbApi.fetchNearbyStations.mockResolvedValue(mockStations);

    // First call
    await locationService.requestNearbyStations();

    // Mock time passing
    locationService._expireCache();

    // Second call after cache expired
    await locationService.requestNearbyStations();

    expect(navigator.geolocation.getCurrentPosition).toHaveBeenCalledTimes(2);
    expect(sbbApi.fetchNearbyStations).toHaveBeenCalledTimes(2);
  });

  test('requestNearbyStations returns cached data on GPS error', async () => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    // First successful call
    navigator.geolocation.getCurrentPosition.mockImplementationOnce((success) => {
      success(mockPosition);
    });
    sbbApi.fetchNearbyStations.mockResolvedValue(mockStations);
    await locationService.requestNearbyStations();

    // Clear cache to force GPS call
    locationService._expireCache();

    // Second call with GPS error
    navigator.geolocation.getCurrentPosition.mockImplementationOnce((success, error) => {
      error({ code: 1, message: 'User denied' });
    });

    const result = await locationService.requestNearbyStations();

    expect(result).toEqual(mockStations);
  });

  test('requestNearbyStations throws error when no cache and GPS fails', async () => {
    navigator.geolocation.getCurrentPosition.mockImplementation((success, error) => {
      error({ code: 1, message: 'User denied' });
    });

    await expect(
      locationService.requestNearbyStations()
    ).rejects.toThrow();
  });
});
```

**Verify RED - Step 2: Run test to verify it fails**

```bash
npm test location_service.test.js
```

Expected: FAIL with "Cannot find module '../src/pkjs/location_service'"

**GREEN - Step 3: Create minimal implementation**

Create `src/pkjs/location_service.js`:

```javascript
const sbbApi = require('./sbb_api');

let lastKnownStations = [];
let lastGPSTime = 0;
const GPS_CACHE_DURATION = 10 * 60 * 1000; // 10 minutes

async function requestNearbyStations() {
  const now = Date.now();

  // Return cached stations if recent
  if (lastKnownStations.length > 0 && (now - lastGPSTime) < GPS_CACHE_DURATION) {
    return lastKnownStations;
  }

  // Request GPS location
  return new Promise((resolve, reject) => {
    navigator.geolocation.getCurrentPosition(
      async (position) => {
        const lat = position.coords.latitude;
        const lon = position.coords.longitude;

        try {
          const stations = await sbbApi.fetchNearbyStations(lat, lon);
          lastKnownStations = stations;
          lastGPSTime = now;
          resolve(stations);
        } catch (err) {
          // If error but have cached data, return cached
          if (lastKnownStations.length > 0) {
            resolve(lastKnownStations);
          } else {
            reject(err);
          }
        }
      },
      (error) => {
        // Return cached stations if available
        if (lastKnownStations.length > 0) {
          resolve(lastKnownStations);
        } else {
          reject(error);
        }
      },
      {
        timeout: 10000,
        maximumAge: GPS_CACHE_DURATION,
        enableHighAccuracy: false
      }
    );
  });
}

// Test helpers
function _clearCache() {
  lastKnownStations = [];
  lastGPSTime = 0;
}

function _expireCache() {
  lastGPSTime = 0;
}

module.exports = {
  requestNearbyStations,
  _clearCache,
  _expireCache
};
```

**Verify GREEN - Step 4: Run test to verify it passes**

```bash
npm test location_service.test.js
```

Expected: All tests PASS

**REFACTOR - Step 5: Clean up (if needed)**

(Code is minimal and clear)

**Step 6: Commit**

```bash
git add src/pkjs/location_service.js tests/location_service.test.js
git commit -m "feat: add GPS location service with caching

TDD: All tests passing
- Fetch GPS and nearby stations
- 10-minute cache
- Fallback to cached data on error
- Error handling"
```

---

### Task 5: AppMessage Handler

**Files:**
- Create: `src/pkjs/message_handler.js`
- Create: `tests/message_handler.test.js`

**RED - Step 1: Write failing tests for message handling**

Create `tests/message_handler.test.js`:

```javascript
const messageHandler = require('../src/pkjs/message_handler');

// Mock dependencies
jest.mock('../src/pkjs/sbb_api');
jest.mock('../src/pkjs/location_service');

const sbbApi = require('../src/pkjs/sbb_api');
const locationService = require('../src/pkjs/location_service');

// Mock Pebble
global.Pebble = {
  sendAppMessage: jest.fn()
};

describe('Message Handler', () => {
  beforeEach(() => {
    Pebble.sendAppMessage.mockClear();
    sbbApi.fetchConnections.mockClear();
    locationService.requestNearbyStations.mockClear();
  });

  test('handleNearbyStationsRequest sends stations to watch', async () => {
    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 },
      { id: '8503006', name: 'Zürich Stadelhofen', distance: 800 }
    ];

    locationService.requestNearbyStations.mockResolvedValue(mockStations);

    Pebble.sendAppMessage.mockImplementation((msg, success) => {
      success();
    });

    const event = {
      payload: { REQUEST_NEARBY_STATIONS: 1 }
    };

    await messageHandler.handleAppMessage(event);

    expect(locationService.requestNearbyStations).toHaveBeenCalled();
    expect(Pebble.sendAppMessage).toHaveBeenCalledTimes(2);
    expect(Pebble.sendAppMessage).toHaveBeenCalledWith(
      expect.objectContaining({
        STATION_ID: '8503000',
        STATION_NAME: 'Zürich HB',
        STATION_DISTANCE: 1200
      }),
      expect.any(Function),
      expect.any(Function)
    );
  });

  test('handleConnectionsRequest sends connection data to watch', async () => {
    const mockConnections = [
      {
        departureTime: 1699362720,
        arrivalTime: 1699367220,
        totalDelayMinutes: 3,
        numChanges: 0,
        sections: [
          {
            platform: '7',
            trainType: 'IC 712',
            delayMinutes: 3
          }
        ]
      }
    ];

    sbbApi.fetchConnections.mockResolvedValue(mockConnections);

    Pebble.sendAppMessage.mockImplementation((msg, success) => {
      success();
    });

    const event = {
      payload: {
        REQUEST_CONNECTIONS: 1,
        DEPARTURE_STATION_ID: '8503000',
        ARRIVAL_STATION_ID: '8507000'
      }
    };

    await messageHandler.handleAppMessage(event);

    expect(sbbApi.fetchConnections).toHaveBeenCalledWith('8503000', '8507000');
    expect(Pebble.sendAppMessage).toHaveBeenCalledWith(
      expect.objectContaining({
        CONNECTION_DATA: 1,
        DEPARTURE_TIME: 1699362720,
        ARRIVAL_TIME: 1699367220,
        PLATFORM: '7',
        TRAIN_TYPE: 'IC 712',
        DELAY_MINUTES: 3,
        NUM_CHANGES: 0
      }),
      expect.any(Function),
      expect.any(Function)
    );
  });

  test('handleAppMessage sends error on GPS failure', async () => {
    locationService.requestNearbyStations.mockRejectedValue(
      new Error('GPS unavailable')
    );

    Pebble.sendAppMessage.mockImplementation((msg, success) => {
      success();
    });

    const event = {
      payload: { REQUEST_NEARBY_STATIONS: 1 }
    };

    await messageHandler.handleAppMessage(event);

    expect(Pebble.sendAppMessage).toHaveBeenCalledWith(
      expect.objectContaining({
        ERROR_MESSAGE: expect.stringContaining('GPS')
      }),
      expect.any(Function),
      expect.any(Function)
    );
  });

  test('handleAppMessage sends error on invalid station IDs', async () => {
    Pebble.sendAppMessage.mockImplementation((msg, success) => {
      success();
    });

    const event = {
      payload: {
        REQUEST_CONNECTIONS: 1,
        DEPARTURE_STATION_ID: '',
        ARRIVAL_STATION_ID: '8507000'
      }
    };

    await messageHandler.handleAppMessage(event);

    expect(Pebble.sendAppMessage).toHaveBeenCalledWith(
      expect.objectContaining({
        ERROR_MESSAGE: expect.stringContaining('Invalid')
      }),
      expect.any(Function),
      expect.any(Function)
    );
  });
});
```

**Verify RED - Step 2: Run test to verify it fails**

```bash
npm test message_handler.test.js
```

Expected: FAIL with "Cannot find module '../src/pkjs/message_handler'"

**GREEN - Step 3: Create minimal implementation**

Create `src/pkjs/message_handler.js`:

```javascript
const sbbApi = require('./sbb_api');
const locationService = require('./location_service');

async function handleAppMessage(event) {
  const message = event.payload;

  if (message.REQUEST_NEARBY_STATIONS !== undefined) {
    await handleNearbyStationsRequest();
  } else if (message.REQUEST_CONNECTIONS !== undefined) {
    await handleConnectionsRequest(
      message.DEPARTURE_STATION_ID,
      message.ARRIVAL_STATION_ID
    );
  }
}

async function handleNearbyStationsRequest() {
  try {
    const stations = await locationService.requestNearbyStations();

    // Send stations one at a time
    for (const station of stations) {
      await sendMessage({
        STATION_ID: station.id,
        STATION_NAME: station.name,
        STATION_DISTANCE: station.distance
      });
    }
  } catch (err) {
    sendError('GPS unavailable. Check location settings.');
  }
}

async function handleConnectionsRequest(fromId, toId) {
  if (!fromId || !toId) {
    sendError('Invalid station IDs');
    return;
  }

  try {
    const connections = await sbbApi.fetchConnections(fromId, toId);

    if (connections.length === 0) {
      sendError('No connections found');
      return;
    }

    // Send first connection
    const conn = connections[0];
    await sendMessage({
      CONNECTION_DATA: 1,
      DEPARTURE_TIME: conn.departureTime,
      ARRIVAL_TIME: conn.arrivalTime,
      PLATFORM: conn.sections[0].platform,
      TRAIN_TYPE: conn.sections[0].trainType,
      DELAY_MINUTES: conn.totalDelayMinutes,
      NUM_CHANGES: conn.numChanges
    });
  } catch (err) {
    sendError('Network error. Check connection.');
  }
}

function sendMessage(message) {
  return new Promise((resolve, reject) => {
    Pebble.sendAppMessage(
      message,
      () => resolve(),
      () => reject(new Error('Failed to send message'))
    );
  });
}

function sendError(message) {
  sendMessage({ ERROR_MESSAGE: message });
}

module.exports = {
  handleAppMessage
};
```

**Verify GREEN - Step 4: Run test to verify it passes**

```bash
npm test message_handler.test.js
```

Expected: All tests PASS

**REFACTOR - Step 5: Clean up (if needed)**

(Code is clear and minimal)

**Step 6: Create main JS file**

Create `src/pkjs/index.js`:

```javascript
const messageHandler = require('./message_handler');

Pebble.addEventListener('ready', () => {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', (event) => {
  messageHandler.handleAppMessage(event);
});
```

**Step 7: Commit**

```bash
git add src/pkjs/message_handler.js src/pkjs/index.js tests/message_handler.test.js
git commit -m "feat: add AppMessage handler with comprehensive tests

TDD: All tests passing
- Handle nearby stations request
- Handle connections request
- Error handling for GPS/network
- Invalid input validation"
```

---

## Phase 3: Watch-Side UI (C) - Manual Testing in Emulator

**Note:** UI code is difficult to unit test in C. We'll build incrementally and test in emulator.

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

**Step 3: Update main.c**

Update `src/main.c`:

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

**Step 4: Test in emulator**

```bash
pebble build
pebble install --emulator aplite
```

**Expected:** Main window displays with "Add Connection" prompt

**Step 5: Commit**

```bash
git add src/main_window.h src/main_window.c src/main.c
git commit -m "feat: add main menu window

Manual test in emulator: PASS
- Shows saved connections list
- Shows 'Add Connection' when empty
- Loads connections from persistence"
```

---

## Continuation Plan

Due to length constraints, the remaining tasks (7-14) follow the same TDD pattern:

**For JavaScript code:**
- RED: Write failing Jest test
- Verify RED: Run `npm test`, confirm failure
- GREEN: Write minimal code
- Verify GREEN: Run `npm test`, confirm pass
- REFACTOR: Clean up
- Commit with test status

**For C code (UI):**
- Write code incrementally
- Test in emulator after each window
- Commit with emulator test status
- Document what was tested

**For integration:**
- Test end-to-end flows in emulator
- Verify AppMessage communication
- Test with mock phone responses

## Remaining Tasks Summary

7. Station Selection Window (C - emulator test)
8. Add Connection Flow (C - emulator test)
9. Connection Detail Window (C - emulator test)
10. Complete AppMessage Handler in C (C - emulator test)
11. Emulator Testing (Integration - full flow test)
12. Error Handling (JS tests + C emulator)
13. README (Documentation)
14. Final Build and Verification (Full integration test)

---

## TDD Verification Checklist

Before marking ANY task complete:

- [ ] Every function has a test (for JS code)
- [ ] Watched each test fail before implementing
- [ ] Each test failed for expected reason
- [ ] Wrote minimal code to pass
- [ ] All tests pass
- [ ] No warnings or errors in output
- [ ] Committed with test status in message

**For C UI code tested in emulator:**
- [ ] Built without warnings
- [ ] Tested specific feature in emulator
- [ ] Feature works as expected
- [ ] No crashes or glitches
- [ ] Committed with emulator test notes

---

**Plan Status:** Complete and ready for TDD execution with @superpowers:subagent-driven-development
