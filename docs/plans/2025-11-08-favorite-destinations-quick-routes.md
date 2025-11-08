# Favorite Destinations and Quick Routes Design

**Date:** 2025-11-08
**Platform:** Pebble 2 (aplite - 144x168 black & white display)
**Feature:** Add favorite destinations for ad-hoc route planning

## Problem Statement

Current UX requires GPS proximity to both departure AND arrival stations to add a connection. This creates poor UX when planning routes from current location to common destinations (Home, Work, Gym, etc.).

**Example scenario:** User is at a random station and wants to go home. Currently they must:
1. Use GPS to find nearby station (works)
2. Use GPS to find "Home" station (fails - not nearby)

## Solution Overview

Add **favorite destinations** that users configure once in phone settings. On watch, users can create **quick routes** from any nearby station to a favorite destination. After viewing, users can optionally save frequently-used routes.

**User Flow:**
1. Configure favorites in Pebble app settings (phone): "Home", "Work", "Gym"
2. On watch: Quick Route → GPS finds nearby stations → Pick departure → Pick favorite destination
3. View trains in real-time
4. Optional: Save this specific route for future quick access

## Data Model

### New Structure: FavoriteDestination

```c
typedef struct {
    char id[MAX_STATION_ID_LENGTH];           // SBB station ID
    char name[MAX_STATION_NAME_LENGTH];       // "Zürich HB"
    char label[16];                            // "Home", "Work", etc.
} FavoriteDestination;

#define MAX_FAVORITE_DESTINATIONS 10
```

### Existing Structure: SavedConnection (Unchanged)

```c
typedef struct {
    char departure_station_id[MAX_STATION_ID_LENGTH];
    char departure_station_name[MAX_STATION_NAME_LENGTH];
    char arrival_station_id[MAX_STATION_ID_LENGTH];
    char arrival_station_name[MAX_STATION_NAME_LENGTH];
} SavedConnection;
```

### Persistence Keys

```c
#define PERSIST_KEY_FAVORITE_DESTINATIONS 5
#define PERSIST_KEY_NUM_FAVORITES 6
```

## Phone Configuration Page

### Purpose
Web UI in Pebble phone app for managing favorite destinations.

### UI Components

**1. Favorite Destinations List**
- Shows: "Home - Zürich HB", "Work - Bern", etc.
- Delete button (X) for each favorite
- "Add Favorite" button at bottom

**2. Add Favorite Form**
- Station search input with autocomplete
  - Calls SBB API `/locations?query={text}` as user types
  - Shows dropdown of matching stations
- Label input: "What do you call this place?"
  - Suggested labels: Home, Work, School, Gym
  - Custom text allowed (max 15 chars)
- Save button

**3. Data Synchronization**
- On page load: Watch sends current favorites via AppMessage
- On save: Page sends updated favorites array to watch
- Watch persists using `save_favorites()` function

### Technical Implementation

- **Hosting:** GitHub Pages or data URI in appinfo.json
- **No backend required:** Pure HTML/JS/CSS
- **Pebble API:**
  - `Pebble.addEventListener('ready')` - receive current favorites
  - `Pebble.sendAppMessage()` - send updated favorites

## Watch UI Changes

### Main Menu (Modified)

**Current behavior:** Shows saved connections list

**New behavior:**
- Shows saved connections (existing)
- Adds "Quick Route" option (long-press UP button or menu item)
- If no favorites configured: Show "Configure favorites in settings" helper text

### Quick Route Window (New)

**Purpose:** Select favorite destination to view trains from nearby station

**Flow:**
1. User taps "Quick Route"
2. GPS searches for nearby stations (reuse existing station_select_window)
3. User selects departure station
4. **New: Favorite Destinations Menu** appears
5. User selects favorite (e.g., "Home")
6. Connection detail window shows trains

**Favorite Destinations Menu:**
- Header: "Travel to..."
- Rows: Show favorite labels ("Home", "Work", "Gym")
- If no favorites: Show "No favorites configured. Open Pebble app settings."

### Connection Detail Window (Redesigned)

**Current layout:** Single-line rows with all info crammed

**New layout:** Multi-line rows with clear hierarchy

```
┌─────────────────────────────┐
│ Zürich Stadelhofen → Home   │  ← Window header
├─────────────────────────────┤
│ IC 712 | Pl.7                │  ← Small header (train + platform)
│ 14:32 → 15:47                │  ← Large time (most prominent)
│ +3 min delay                 │  ← Small footer (delay status)
├─────────────────────────────┤
│ RE 4521 | Pl.3               │
│ 14:45 → 16:02                │
│ On time ✓                    │
└─────────────────────────────┘
```

**Visual Indicators (Black & White):**
- On time: `✓` checkmark
- Delayed: `+X min`
- Cancelled: Inverse video (white text on black)
- Platform change: `Pl.7→9`
- Tight transfer (<5 min): `1 chg !`

**Scrolling Text:**
- When row is selected and text exceeds width, animate horizontal scroll
- Apply to train number/name line if truncated
- Use `menu_selection_changed` callback to trigger animation

**Action Menu (Long-press UP):**
- "Save Route" - Persists current quick route as saved connection
- "Refresh" - Re-fetch connection data

### Save Route Action (New)

**Trigger:** Long-press UP while viewing quick route → Select "Save Route"

**Behavior:**
1. Check if max saved connections reached (10)
   - If yes: Show error "Maximum 10 saved routes. Delete one first."
2. Check if route already saved
   - If yes: Ask "Route already saved. Update it?"
3. Save to persistence using existing `save_connections()`
4. Show confirmation: "Route saved"
5. Return to main menu (route now appears in saved list)

## AppMessage Protocol

### New Message Keys (appinfo.json)

```json
{
  "FAVORITE_DESTINATION_ID": 50,
  "FAVORITE_DESTINATION_NAME": 51,
  "FAVORITE_DESTINATION_LABEL": 52,
  "REQUEST_QUICK_ROUTE": 53,
  "SAVE_CURRENT_ROUTE": 54,
  "CONFIG_SYNC_FAVORITES": 55
}
```

### Message Flows

**1. Config Page → Watch (Save Favorites)**

Multiple messages, one per favorite:
```javascript
Pebble.sendAppMessage({
  FAVORITE_DESTINATION_ID: "8503000",
  FAVORITE_DESTINATION_NAME: "Zürich HB",
  FAVORITE_DESTINATION_LABEL: "Home"
});
```

Watch handler:
- Receives all favorites
- Calls `save_favorites(favorites, count)`
- Confirms via callback

**2. Watch → Phone (Request Quick Route)**

```c
DictionaryIterator *iter;
app_message_outbox_begin(&iter);
dict_write_uint8(iter, MESSAGE_KEY_REQUEST_QUICK_ROUTE, 1);
dict_write_cstring(iter, MESSAGE_KEY_DEPARTURE_STATION_ID, "8503003");
dict_write_cstring(iter, MESSAGE_KEY_FAVORITE_DESTINATION_ID, "8503000");
app_message_outbox_send();
```

Phone handler:
- Calls `/connections?from={departure}&to={favorite}`
- Returns connection data (reuses existing CONNECTION_DATA message)

**3. Watch → Phone (Save Current Route)**

Optional: Could be watch-only operation, no phone communication needed.

## JavaScript Changes

### New File: src/pkjs/config.html

Configuration page for managing favorites.

**Dependencies:**
- SBB API client (reuse `fetchNearbyStations` for autocomplete)
- Pebble.js for AppMessage communication

### Modified: src/pkjs/message_handler.js

Add handler for `REQUEST_QUICK_ROUTE`:

```javascript
function handleQuickRouteRequest(departureId, favoriteId) {
    sbbApi.fetchConnections(departureId, favoriteId, (err, connections) => {
        if (err) {
            sendError('Could not find route');
            return;
        }
        // Send connection data (reuse existing logic)
        sendConnectionData(connections);
    });
}
```

### Modified: src/pkjs/sbb_api.js

No changes needed - `fetchConnections()` already accepts any station IDs.

## Error Handling

### Favorite Destinations

| Error | Handling |
|-------|----------|
| No favorites configured | Show "Configure favorites in Pebble app settings" when Quick Route tapped |
| Max favorites reached (10) | Config page disables "Add Favorite" button, shows warning |
| Favorite station not found | API returns error, show "Station not found" dialog |
| Favorite name too long | Config page truncates to 15 chars with "..." |

### Quick Route Flow

| Error | Handling |
|-------|----------|
| GPS timeout/failure | Show cached nearby stations from last GPS (existing behavior) |
| No nearby stations | Show "No stations found. Move closer to a train station." |
| No connections available | Show "No trains found for this route" with timestamp |
| Network timeout | Reuse existing error dialog system |

### Connection Display

| Error | Handling |
|-------|----------|
| Very long delay (>60 min) | Show "+60+ min" |
| Cancelled train | Show "CANCELLED" in inverse video (white on black) |
| Platform change | Show "Pl.7→9" |
| Tight transfer (<5 min) | Show "!" next to change count |

### Save Route

| Error | Handling |
|-------|----------|
| Max saved connections (10) | Show "Maximum 10 saved routes. Delete one first." |
| Duplicate route exists | Ask "Route already saved. Update it?" |

## Implementation Files

### New Files

- `src/pkjs/config.html` - Configuration page UI
- `src/quick_route_window.h/c` - Favorite destinations menu
- `docs/plans/2025-11-08-favorite-destinations-implementation.md` - Implementation plan

### Modified Files

- `src/data_models.h/c` - Add FavoriteDestination structure
- `src/persistence.h/c` - Add save/load favorites functions
- `src/main_window.c` - Add "Quick Route" action
- `src/connection_detail_window.h/c` - Redesign row layout, add scrolling text
- `src/app_message.c` - Add handlers for favorite sync and quick routes
- `src/pkjs/message_handler.js` - Add REQUEST_QUICK_ROUTE handler
- `appinfo.json` - Add new message keys, enable configurable capability

## Testing Strategy

### Unit Tests

**JavaScript (Jest):**
- Config page favorites sync
- Quick route request handling
- API integration with favorite destinations

**C (Manual/Emulator):**
- Favorite persistence (save/load)
- Quick route window navigation
- Connection detail rendering with new layout

### Integration Tests

- End-to-end: Configure favorite → Quick route → View trains → Save route
- GPS failure scenarios with favorites
- Max limits (10 favorites, 10 saved routes)
- Text scrolling on selected rows

### Real-world Testing

- Configure favorites at home, test quick routes at various stations
- Verify connection data accuracy against SBB displays
- Test with very long station names (scrolling text)
- Battery impact of configuration page

## Migration Strategy

**Existing users:** No data migration needed
- Saved connections continue to work unchanged
- Favorites start empty (users configure as needed)
- Quick Route is optional new feature

**New users:**
- App guides to configure favorites on first launch
- Can use either saved connections OR quick routes
- Most users will likely use both patterns

## Success Criteria

1. ✅ Users can configure favorite destinations via phone settings
2. ✅ Quick route from any nearby station to favorite works
3. ✅ Connection detail window shows clear, scannable information
4. ✅ Text scrolling reveals truncated content on watch
5. ✅ Save route action converts quick route to saved connection
6. ✅ No regressions to existing saved connections feature

## Future Enhancements (Out of Scope)

- Voice input for favorite labels
- Import favorites from calendar locations
- "Recent destinations" auto-populated from quick routes
- Reverse route (tap to swap departure/arrival)
- Time-based favorites (different "Work" station for weekday vs weekend)
