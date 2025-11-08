# Favorites as Departure Stations - Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Allow users to select favorite stations as departure points without GPS, solving the testing problem when GPS signal is weak.

**Architecture:** Modify station_select_window.c to load FavoriteDestinations, show "Stations near me" as explicit trigger, and convert favorites to Station format for display.

**Tech Stack:** Pebble C SDK, AppMessage system

**Design Document:** docs/plans/2025-11-08-favorites-as-departure-stations.md

---

## Task 1: Add GPS Search State and Conversion Function

**Files:**
- Modify: `src/station_select_window.c:8-15`

**Step 1: Add state tracking variable**

After line 14 (`static StationSelectCallback s_callback;`), add:

```c
static bool s_gps_search_active = false;
```

This tracks whether the user has triggered GPS search.

**Step 2: Add favorite-to-station conversion function**

After the forward declarations (around line 26), add:

```c
// Convert FavoriteDestination to Station for display
static Station favorite_to_station(FavoriteDestination *fav) {
    return create_station(fav->id, fav->name, 0);  // distance = 0 for favorites
}
```

**Step 3: Build and verify compilation**

Run:
```bash
pebble build
```

Expected: Build succeeds with no errors

**Step 4: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: add GPS search state tracking and favorite conversion function"
```

---

## Task 2: Modify Menu Structure for "Stations Near Me" Row

**Files:**
- Modify: `src/station_select_window.c:53-62`

**Step 1: Update section count logic**

Replace `menu_get_num_sections_callback` function (lines 53-55):

```c
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    // Always show 2 sections: Action/Nearby + Favorites
    return 2;
}
```

**Step 2: Update row count logic**

Replace `menu_get_num_rows_callback` function (lines 57-62):

```c
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
```

**Step 3: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds

**Step 4: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: update menu structure for stations near me row"
```

---

## Task 3: Update Menu Headers

**Files:**
- Modify: `src/station_select_window.c:64-74`

**Step 1: Update header drawing logic**

Replace `menu_draw_header_callback` function (lines 68-74):

```c
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    if (section_index == 0) {
        if (s_num_stations > 0) {
            menu_cell_basic_header_draw(ctx, cell_layer, "Nearby Stations");
        } else {
            menu_cell_basic_header_draw(ctx, cell_layer, "Select Departure");
        }
    } else {
        menu_cell_basic_header_draw(ctx, cell_layer, "★ Favorites");
    }
}
```

**Step 2: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: update menu headers for new structure"
```

---

## Task 4: Update Row Drawing Logic

**Files:**
- Modify: `src/station_select_window.c:76-126`

**Step 1: Replace menu_draw_row_callback function**

Replace the entire `menu_draw_row_callback` function (lines 76-126) with:

```c
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
            menu_cell_basic_draw(ctx, cell_layer, "Stations near me", "Search by GPS →", NULL);
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
```

**Step 2: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: update row drawing for stations near me and favorites"
```

---

## Task 5: Update Selection Callback

**Files:**
- Modify: `src/station_select_window.c:137-151`

**Step 1: Replace menu_select_callback function**

Replace the entire `menu_select_callback` function (lines 137-151) with:

```c
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
```

**Step 2: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: handle stations near me selection and GPS trigger"
```

---

## Task 6: Load FavoriteDestinations and Remove Auto GPS

**Files:**
- Modify: `src/station_select_window.c:153-190`

**Step 1: Update window_load to use FavoriteDestinations**

Replace the `window_load` function (lines 153-190):

```c
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
```

**Step 2: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: load favorite destinations and remove auto GPS trigger"
```

---

## Task 7: Reset GPS State on Window Push

**Files:**
- Modify: `src/station_select_window.c:201-213`

**Step 1: Update station_select_window_push to reset GPS state**

Replace the `station_select_window_push` function (lines 201-213):

```c
void station_select_window_push(StationSelectCallback callback) {
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
```

**Step 2: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds with no warnings

**Step 3: Commit**

```bash
git add src/station_select_window.c
git commit -m "feat: reset GPS state when opening station selection"
```

---

## Task 8: Remove Debug Logging from Previous Session

**Files:**
- Modify: `src/main_window.c:75-79`
- Modify: `src/quick_route_window.c:70-72`

**Step 1: Remove debug logs from main_window.c**

Replace lines 75-79 in `src/main_window.c`:

```c
static void quick_route_departure_selected(Station *station) {
    quick_route_window_push(station, quick_route_selected_callback);
}
```

**Step 2: Remove debug log from quick_route_window.c**

Replace lines 70-72 in `src/quick_route_window.c`:

```c
    // Load favorites
    s_num_favorites = load_favorite_destinations(s_favorites);
}
```

**Step 3: Build and verify**

Run:
```bash
pebble build
```

Expected: Build succeeds

**Step 4: Commit**

```bash
git add src/main_window.c src/quick_route_window.c
git commit -m "chore: remove debug logging from previous session"
```

---

## Task 9: Manual Testing

**Testing checklist:**

**Test 1: Favorites appear first**
1. Install app on watch: `pebble install --phone <IP>`
2. Long-press UP to start quick route
3. Verify menu shows:
   - "Stations near me" at top
   - "★ Favorites" section below
   - Your 4 favorites listed (ZHAW, Winkel, Belimo, Home)

Expected: All favorites visible without GPS

**Test 2: GPS search works**
1. Select "Stations near me"
2. Verify menu changes to "Searching..."
3. Wait for GPS results
4. Verify "Nearby Stations" section appears above favorites

Expected: GPS stations load, favorites remain visible

**Test 3: Select favorite without GPS**
1. Open quick route
2. Select a favorite directly (e.g., "Home")
3. Verify quick route destination window opens with favorites list

Expected: Can complete quick route without using GPS

**Test 4: Indoor/weak GPS scenario**
1. Go indoors or disable location services
2. Open quick route
3. Select "Stations near me"
4. Wait for timeout/error
5. Verify favorites are still selectable

Expected: Can fallback to favorites when GPS fails

**Step: Document test results**

Create file `docs/testing-notes-favorites-departure.md` with results of each test.

**Step: Final commit**

```bash
git add docs/testing-notes-favorites-departure.md
git commit -m "test: manual testing results for favorites as departure"
```

---

## Task 10: Final Cleanup and Merge

**Step 1: Review all changes**

Run:
```bash
git log --oneline master..HEAD
```

Expected: See all commits from this implementation

**Step 2: Create summary commit message**

Prepare summary of changes for merge/PR

**Step 3: Optional - Create PR or merge**

If user wants to merge:
```bash
git checkout master
git merge --no-ff <branch-name> -m "feat: add favorites as departure station options"
```

---

## Notes

- **DRY:** Reuse existing conversion pattern from data_models
- **YAGNI:** Don't add favorite editing or reordering in this task
- **Testing:** Manual testing required - Pebble has no unit test framework
- **Commits:** Small, focused commits after each working change
