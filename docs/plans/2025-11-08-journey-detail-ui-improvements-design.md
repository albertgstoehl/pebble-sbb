# Journey Detail UI Improvements - Design Document

**Date**: 2025-11-08
**Status**: Approved

## Overview

This design adds multi-section journey display, connection pinning, and consistent text scrolling to the Pebble SBB Schedule app. Users can view detailed train connections showing all transfer points, pin active journeys for quick access, and save frequently used routes.

## Problem Statement

The app currently shows train connections as single summary items. Users cannot see transfer details, long station names are truncated without scrolling, and there is no way to pin an active journey for reference while traveling.

## Goals

1. Display multi-section journeys with visual connection timeline
2. Pin active connections to main menu (auto-expire on arrival)
3. Save connection routes (station A → B) to persistent storage
4. Apply horizontal text scrolling consistently across all menus
5. Fix favorites display inconsistency between departure and destination windows

## Architecture

### Window Navigation Flow

```
Main Menu (main_window.c)
  ├─ Section 0: Active Journey (when pinned) → Journey Detail
  └─ Section 1: Saved Connections → Connection List

Connection List (connection_detail_window.c)
  └─ Shows 5 connections as summaries → Journey Detail (on select)

Journey Detail (NEW: journey_detail_window.c)
  └─ Shows multi-section breakdown with timeline graphics
```

### New Components

**journey_detail_window.c** + **.h**
- Displays single connection with all sections as menu rows
- Custom graphics drawing (circles, vertical lines)
- Horizontal text scrolling for long station names
- Long-press handlers for save/pin actions

**pinned_connection.c** + **.h**
- Manages active pinned connection
- Handles expiry on arrival time
- Persistence functions

### Modified Components

**main_window.c**
- Add "Active Journey" section (visible only when connection is pinned)
- Update menu callbacks to handle two sections

**connection_detail_window.c**
- Add select callback to push journey_detail_window
- Add long-press SELECT handler (save connection)
- Add long-press DOWN handler (pin connection)
- Add text scrolling for connection summaries

**quick_route_window.c**
- Fix favorites display: show label + name (match station_select_window)
- Add text scrolling for destination favorites

**station_select_window.c**
- No changes (scrolling already implemented)

## Data Structures

### PinnedConnection (new)

```c
typedef struct {
    Connection connection;          // Full connection with all sections
    SavedConnection route;          // Route info (A→B stations)
    time_t pinned_at;              // When user pinned it
    bool is_active;                // Whether a connection is pinned
} PinnedConnection;
```

Stored in persistent storage. Checked on app launch and during runtime. Cleared automatically when `connection.arrival_time < current_time`.

## Journey Detail Window Design

### Layout

MenuLayer with custom row drawing (hybrid approach):
- **Header row**: Connection summary (departure → arrival, total time, changes)
- **Section rows**: One row per journey section

Each section row (70px height):
```
┌─────────────────────────────────┐
│ ● Zürich HB          | Pl. 4   │  ← Departure: filled circle + station + platform
│ │ IC 1  →  14:23               │  ← Vertical line + train type + time
│ │                               │  ← Line continues
│ ○ Winterthur         | Pl. 2   │  ← Arrival: hollow circle + station + platform
│   Arr: 14:45  | +2 min          │  ← Arrival time + delay
└─────────────────────────────────┘
```

### Graphics Drawing

Using Pebble SDK graphics primitives:
- `graphics_fill_circle(ctx, GPoint(8, 10), 4)` - Departure marker (filled)
- `graphics_draw_circle(ctx, GPoint(8, 50), 4)` - Arrival marker (outline)
- `graphics_draw_line(ctx, GPoint(8, 14), GPoint(8, 46))` - Vertical timeline
- `graphics_draw_text()` - Station names, times, platforms
- All drawn in `GColorBlack` on `GColorWhite` background

### Special Cases

- **Last section**: No vertical line extends beyond arrival circle
- **Transfers**: Arrival of section N matches departure of section N+1
- **Long names**: Horizontal scrolling (timer-based, from station_select_window pattern)

## Button Actions

Available from connection_detail_window and journey_detail_window:

**Long-press SELECT**: Save connection route
1. Extract departure/arrival station IDs and names from Connection
2. Create SavedConnection struct
3. Call `save_connection()` (existing persistence function)
4. Show confirmation: "Connection saved"
5. Main menu reloads to show new saved connection

**Long-press DOWN**: Pin current connection
1. Store entire Connection struct (all sections + timing)
2. Create PinnedConnection with `is_active = true`, `pinned_at = current_time`
3. Call `save_pinned_connection()` (new persistence function)
4. Show confirmation: "Connection pinned"
5. Main menu shows "Active Journey" section on return

## Pinned Connection System

### Main Menu Integration

When `is_active == true`:
- Menu shows 2 sections instead of 1
- Section 0: "Active Journey" header
  - Single row: "{departure} → {arrival} ({departure_time})"
  - Selecting row opens journey_detail_window with pinned connection
- Section 1: "Saved Connections" (existing)

When `is_active == false`:
- Menu shows 1 section
- Section 0: "Saved Connections" (existing behavior)

### Expiry Logic

**On app launch** (main.c initialization):
```c
PinnedConnection pinned = load_pinned_connection();
if (pinned.is_active && pinned.connection.arrival_time < time(NULL)) {
    pinned.is_active = false;
    save_pinned_connection(&pinned);
}
```

**Optional runtime check**: 60-second timer to clear expired pin during use.

## Text Scrolling Implementation

Apply scrolling pattern from station_select_window.c (lines 18-57) to:

1. **connection_detail_window.c**: Connection summaries
2. **journey_detail_window.c**: Journey section rows
3. **quick_route_window.c**: Destination favorites

### Pattern Components

Static variables:
```c
static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset = 0;
static bool s_scrolling_required = false;
static bool s_menu_reloading = false;
```

Functions:
- `scroll_menu_callback(void *data)` - Timer callback, increments offset
- `initiate_menu_scroll_timer()` - Starts scroll after 1-second delay
- In draw callback: Check if selected, calculate if text exceeds ~17 chars, apply offset
- In selection_changed callback: Call `initiate_menu_scroll_timer()`

Constants (already defined):
```c
#define SCROLL_WAIT_MS 1000    // Delay before scrolling starts
#define SCROLL_STEP_MS 200     // Scroll speed
#define MENU_CHARS_VISIBLE 17  // Approx visible characters
```

## Favorites Display Fix

**Current inconsistency**:
- station_select_window.c: Shows label + name correctly
- quick_route_window.c: Different display format

**Fix** (quick_route_window.c):
```c
// For favorites:
menu_cell_basic_draw(ctx, cell_layer, fav->label, fav->name, NULL);
```

Ensures both windows display favorites identically:
- Title: Custom label ("Home", "Work", "ZHAW")
- Subtitle: Full station name ("Winterthur, Hauptbahnhof")

## Implementation Order

1. Create journey_detail_window.c/.h with basic MenuLayer
2. Implement custom drawing (circles, lines, text layout)
3. Add text scrolling to journey_detail_window
4. Create pinned_connection.c/.h with persistence
5. Update main_window.c for two-section menu
6. Add long-press handlers to connection_detail_window
7. Add text scrolling to connection_detail_window
8. Fix favorites display in quick_route_window
9. Add text scrolling to quick_route_window
10. Implement expiry logic in main.c
11. Testing and refinement

## Testing Checklist

- [ ] Journey detail shows all sections with correct graphics
- [ ] Vertical line connects sections, no line after last section
- [ ] Text scrolls horizontally for long station names when selected
- [ ] Long-press SELECT saves connection to main menu
- [ ] Long-press DOWN pins connection, appears in "Active Journey"
- [ ] Pinned connection opens journey detail from main menu
- [ ] Pin auto-expires when arrival time passes
- [ ] Favorites display consistently in departure and destination windows
- [ ] All menu text scrolls when selected and too long
- [ ] No crashes with edge cases (0 sections, 5 sections, very long names)

## Open Questions

None. Design is approved and ready for implementation.
