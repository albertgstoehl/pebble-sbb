# Favorites as Departure Stations Design

**Date:** 2025-11-08
**Status:** Approved

## Problem

When testing the quick route feature indoors or with weak GPS signal, users cannot select a departure station because the station selection window waits for GPS results. This blocks testing and reduces feature usability.

## Solution

Show favorite stations in the departure station selection window, allowing users to select them without GPS. Add an explicit "Stations near me" trigger for GPS search.

## User Interface

### Initial Window (Before GPS Search)

```
┌─────────────────────────┐
│ Select Departure        │
├─────────────────────────┤
│ Stations near me    →   │ <- Triggers GPS
├─────────────────────────┤
│ ★ Favorites             │
├─────────────────────────┤
│ ZHAW                    │
│ Winkel                  │
│ Belimo                  │
│ Home                    │
└─────────────────────────┘
```

### After GPS Search Completes

```
┌─────────────────────────┐
│ Nearby Stations         │
├─────────────────────────┤
│ Winterthur       0.3 km │
│ Zürich HB        2.1 km │
├─────────────────────────┤
│ ★ Favorites             │
├─────────────────────────┤
│ ZHAW                    │
│ Winkel                  │
│ Belimo                  │
│ Home                    │
└─────────────────────────┘
```

## Implementation

### File: `src/station_select_window.c`

**State Changes:**
- Add `s_gps_search_active` boolean flag
- Change `s_favorites` to load `FavoriteDestination` objects
- Add conversion function: `Station favorite_to_station(FavoriteDestination *fav)`

**Menu Structure Logic:**

1. **Section count:**
   - Before GPS: 2 sections (Action + Favorites)
   - After GPS: 2 sections (Nearby + Favorites)

2. **Section 0:**
   - If `!s_gps_search_active && s_num_stations == 0`: Show "Stations near me" row
   - If `s_gps_search_active && s_num_stations == 0`: Show loading message
   - If `s_num_stations > 0`: Show nearby stations with distances

3. **Section 1:**
   - Always show favorites (converted from FavoriteDestination to Station)

**Window Load Changes:**
- Load `FavoriteDestination` objects via `load_favorite_destinations()`
- Convert to Station format for display
- **Remove** automatic GPS trigger (lines 186-189)
- Only send REQUEST_NEARBY_STATIONS when user selects "Stations near me"

**Selection Callback:**
- Section 0, Row 0: If "Stations near me", set `s_gps_search_active = true` and trigger GPS
- Section 0, Other rows: GPS station selected, call callback
- Section 1: Favorite selected, call callback

### Data Conversion

```c
Station favorite_to_station(FavoriteDestination *fav) {
    return create_station(fav->id, fav->name, 0);  // distance = 0
}
```

## Error Handling

**No favorites saved:**
- Show only "Stations near me" option
- If GPS fails, show: "Could not find stations. Configure favorites in settings."

**GPS timeout/failure:**
- Keep favorites visible and selectable
- Show error but don't block UI
- User can still pick a favorite

**Empty results:**
- No favorites AND no GPS: Show helpful message to configure favorites

**Window state:**
- Clear GPS results on `window_unload`
- Each window open starts fresh

## Benefits

1. **Testing:** Can test quick routes without GPS signal
2. **Speed:** Skip GPS wait for common routes (Home → Work, etc.)
3. **Reliability:** Always have fallback when GPS fails
4. **Flexibility:** User chooses when to use GPS vs. favorites

## Migration

Consolidate on `FavoriteDestination` as the single favorites system. The old `Station` favorites system (PERSIST_KEY_FAVORITES) can be deprecated since it's unused.
