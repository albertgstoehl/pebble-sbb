# SBB Train Schedule for Pebble

A Pebble smartwatch app for viewing Swiss train schedules with real-time delay information.

## Features

- Save favorite train connections (e.g., "Home to Work")
- Quick routes from any nearby station to favorite destinations
- Phone configuration page for managing favorites
- GPS-based station selection
- Real-time delay information from SBB OpenData API
- Multi-leg journey support with transfer information
- Auto-refreshing schedule updates every 60 seconds
- Improved connection display with clear three-line layout
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

### Saved Connections

1. Launch app to see saved connections
2. Tap a connection to view upcoming trains
3. Connection refreshes automatically every 60 seconds

### Quick Routes

1. Long-press UP button on main menu
2. GPS finds nearby stations
3. Select your current/departure station
4. Select a favorite destination (Home, Work, etc.)
5. View upcoming trains with real-time delays

### Managing Favorites

1. Open Pebble app on phone
2. Tap Settings for SBB Schedule
3. Search for stations and add as favorites
4. Assign labels like "Home", "Work", "Gym"
5. Favorites sync to watch automatically

### Adding Saved Connections

1. Long-press SELECT button on main menu
2. Select departure station (GPS finds nearby stations)
3. Select arrival station
4. View upcoming trains with real-time delays

## Development

### Project Structure

```
src/
├── main.c                      # App entry point
├── data_models.h/c             # Data structures (includes FavoriteDestination)
├── persistence.h/c             # Local storage (with favorite persistence)
├── app_message.h/c             # Watch-phone communication
├── main_window.h/c             # Main menu
├── connection_detail_window.h/c # Train schedule view (three-line layout)
├── station_select_window.h/c   # Station picker
├── add_connection_window.h/c   # Add connection wizard
├── quick_route_window.h/c      # Favorite destinations picker
├── error_dialog.h/c            # Error display
└── pkjs/
    ├── index.js                # PebbleKit JS entry
    ├── sbb_api.js              # SBB API client (with MOCK_MODE)
    ├── location_service.js     # GPS handler
    ├── message_handler.js      # Message routing
    ├── config.html             # Configuration page UI
    └── config.js               # Configuration page logic
```

### Testing

```bash
# Run JavaScript tests
npm test

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
