# SBB Train Schedule Pebble App Design

**Date:** 2025-11-07
**Platform:** Pebble 2 (aplite - 144x168 black & white display)
**API:** SBB OpenData API

## Overview

This app provides quick access to Swiss train schedules directly from your Pebble watch. Users configure favorite connections (e.g., "Home → Work") on the watch and view upcoming trains with real-time departure information, delays, and platform numbers.

## Core Requirements

- Configure saved connections on the watch (no phone configuration required)
- Display next 3-5 trains for each saved connection
- Show departure/arrival times, platform numbers, train type/number, and real-time delays
- Handle connections with transfers (multi-leg journeys)
- Use GPS to detect nearby stations for easier station selection
- Black and white display (text and symbols only, no color indicators)

## Architecture

### Component Model

The app uses the standard Pebble architecture: PebbleKit JS on the phone handles networking and GPS, while C code on the watch handles UI and persistence.

**Watch-side (C):**
- UI rendering and user input
- Data persistence (persist API)
- Connection list display
- Train schedule views

**Phone-side (JavaScript):**
- HTTP communication with SBB OpenData API
- GPS location services
- JSON parsing and data formatting
- Message passing to/from watch via AppMessage

### Data Flow

1. Watch sends request via AppMessage (connection query or nearby stations)
2. JS component fetches data (API call or GPS lookup)
3. JS processes response and formats for watch
4. Watch receives structured data and updates UI
5. Watch persists saved connections locally

## User Interface

### Main Menu

Displays saved connections as a scrollable list. If no connections exist, shows "Add Connection" prompt.

**Navigation:**
- UP/DOWN: Scroll through connections
- SELECT: Open connection to view trains
- BACK: Exit app
- Long-press UP: Action bar (Add New, Settings)

### Connection Detail View

Shows 3-5 upcoming trains for the selected route. Each entry displays:
- Departure time → Arrival time
- Platform number
- Train type and number (e.g., "IC 712")
- Delay status (✓ on-time, "+5 min" if delayed)

**Format:** `14:32 → 15:47 | Pl.7 | IC 712 | +3 min`

Auto-refreshes every 60 seconds while active. Pressing SELECT on a train shows full details.

### Multi-leg Journeys

Connections with transfers display transfer count in list view:
- `14:32 → 15:47 | 1 chg | Pl.7 | +3 min`

Detail view expands to show each leg:
```
Zürich HB          14:32  Pl.7
→ IC 712
Bern               15:15  Pl.3

Change: 8 min

Bern               15:23  Pl.6
→ RE 4567
Interlaken Ost     16:02  Pl.2
```

Shows "!" indicator if transfer time is less than 5 minutes.

### Adding Connections

Two-step wizard:
1. **Select Departure:** Shows nearby stations (GPS) with favorites marked ★ at top
2. **Select Arrival:** Same interface for destination station

Stations display with distance: `Zürich HB (1.2km)`

Long-press SELECT on any station adds it to favorites.

## AppMessage Protocol

### Message Types

**REQUEST_CONNECTIONS** (watch → phone)
- Payload: departure station ID, arrival station ID
- Response: Array of up to 5 connections with times, platforms, trains, delays

**REQUEST_NEARBY_STATIONS** (watch → phone)
- Payload: None (triggers GPS)
- Response: Sorted array of stations with names, IDs, distances

**CONNECTION_DATA** (phone → watch)
- Payload: Array of connection objects
  - Each connection: array of sections (legs)
  - Each section: departure/arrival times, platform, train designation, delay

**STATION_LIST** (phone → watch)
- Payload: Array of station objects (name, ID, distance in meters)

## Data Persistence

Uses Pebble persist API for local storage:
- Saved connections array (maximum 10 connections)
- User favorites list (stations)
- Connection structs serialized to bytes

## Error Handling

**Network timeout (10s):** Display "Network Error - Retry?" message

**No GPS signal:** Fall back to last known nearby stations or favorites list only

**API rate limiting:** Exponential backoff in JS layer, show "Please wait..." on watch

**No connections available:** Display "No connections available" with timestamp

**Bluetooth disconnected:** Show cached last query results with "Offline" indicator

**Connection cancelled:** Display "CANCELLED" with alternative suggestions if available

## Edge Cases

- **Long station names:** Truncate with ellipsis (e.g., "Interl...")
- **Overnight journeys:** Show date when not today
- **Low battery:** Reduce refresh rate from 60s to 120s
- **Maximum connections reached:** Prompt to delete one before adding
- **Transfer delays:** Propagate delays from first leg to show impact on connection
- **Tight transfers:** Warn when transfer time under 5 minutes

## Testing Strategy

### Unit Testing (TDD)

**Watch-side (C):**
- Test UI rendering with mock data in emulator
- Test menu navigation logic
- Test persistence read/write operations
- Test station name truncation for display limits

**Phone-side (JavaScript):**
- Mock SBB API responses for connection queries
- Test GPS fallback scenarios
- Test message serialization/deserialization
- Test rate limiting and timeout handling

### Emulator Testing

Use Pebble SDK emulator with aplite platform (Pebble 2 display):
```bash
pebble build
pebble install --emulator aplite
```

Simulates 144x168 black and white display for accurate UI testing.

### Integration Testing

- End-to-end flow: GPS request → API call → display
- Test with real SBB API in various Swiss locations
- Verify refresh logic and stale data handling
- Test AppMessage communication under poor Bluetooth conditions

### Real-world Testing

- Test in Swiss train stations with actual GPS coordinates
- Verify platform information accuracy
- Test battery consumption during typical daily use
- Validate delay information against station displays

## Performance Considerations

**Minimize API calls:**
- Cache nearby stations for 10 minutes
- Only refresh connection data when viewing
- Stop auto-refresh when window hidden

**Optimize AppMessage size:**
- Use short message keys
- Send timestamps as integers
- Limit string lengths

**Battery optimization:**
- Request GPS only when adding connections (not continuously)
- Reduce refresh frequency when battery low
- Stop all background activity when app not visible

## Success Criteria

1. Users can add saved connections using only the watch (no phone needed)
2. Real-time train information updates within 2 seconds
3. App remains usable for full day on single charge
4. All delays accurately reflected from SBB API
5. Multi-leg journeys display clearly with transfer information
6. No crashes under normal usage conditions

## Future Enhancements (Out of Scope)

- Voice input for station selection
- Push notifications for train delays
- Integration with Pebble Timeline
- Support for other Swiss transport (trams, buses)
- Journey planning from current location
