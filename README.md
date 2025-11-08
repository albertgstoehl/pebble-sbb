# SBB Train Schedule for Pebble

A Pebble smartwatch app for viewing Swiss train schedules with real-time delay information.

## Features

- Save favorite train connections (e.g., "Home to Work")
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
