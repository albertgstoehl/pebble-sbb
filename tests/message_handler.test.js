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

  test('handleNearbyStationsRequest sends stations to watch', (done) => {
    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 },
      { id: '8503006', name: 'Zürich Stadelhofen', distance: 800 }
    ];

    locationService.requestNearbyStations.mockImplementation((callback) => {
      callback(null, mockStations);
    });

    Pebble.sendAppMessage.mockImplementation((msg, success) => {
      success();
    });

    const event = {
      payload: { REQUEST_NEARBY_STATIONS: 1 }
    };

    messageHandler.handleAppMessage(event);

    // Wait a tick for the async callback to execute
    setTimeout(() => {
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
      done();
    }, 10);
  });

  test('handleConnectionsRequest sends connection data to watch', (done) => {
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

    sbbApi.fetchConnections.mockImplementation((fromId, toId, callback) => {
      callback(null, mockConnections);
    });

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

    messageHandler.handleAppMessage(event);

    setTimeout(() => {
      expect(sbbApi.fetchConnections).toHaveBeenCalledWith('8503000', '8507000', expect.any(Function));
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
      done();
    }, 10);
  });

  test('handleAppMessage sends error on GPS failure', (done) => {
    locationService.requestNearbyStations.mockImplementation((callback) => {
      callback(new Error('GPS unavailable'), null);
    });

    Pebble.sendAppMessage.mockImplementation((msg, success) => {
      success();
    });

    const event = {
      payload: { REQUEST_NEARBY_STATIONS: 1 }
    };

    messageHandler.handleAppMessage(event);

    setTimeout(() => {
      expect(Pebble.sendAppMessage).toHaveBeenCalledWith(
        expect.objectContaining({
          ERROR_MESSAGE: expect.stringContaining('GPS')
        }),
        expect.any(Function),
        expect.any(Function)
      );
      done();
    }, 10);
  });

  test('handleAppMessage sends error on invalid station IDs', (done) => {
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

    messageHandler.handleAppMessage(event);

    setTimeout(() => {
      expect(Pebble.sendAppMessage).toHaveBeenCalledWith(
        expect.objectContaining({
          ERROR_MESSAGE: expect.stringContaining('Invalid')
        }),
        expect.any(Function),
        expect.any(Function)
      );
      done();
    }, 10);
  });
});
