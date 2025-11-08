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

  test('requestNearbyStations fetches GPS and returns stations', (done) => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    navigator.geolocation.getCurrentPosition.mockImplementation((success) => {
      success(mockPosition);
    });

    sbbApi.fetchNearbyStations.mockImplementation((lat, lon, callback) => {
      callback(null, mockStations);
    });

    locationService.requestNearbyStations((err, result) => {
      expect(err).toBeNull();
      expect(result).toEqual(mockStations);
      expect(navigator.geolocation.getCurrentPosition).toHaveBeenCalled();
      expect(sbbApi.fetchNearbyStations).toHaveBeenCalledWith(47.3769, 8.5417, expect.any(Function));
      done();
    });
  });

  test('requestNearbyStations uses cached data within 10 minutes', (done) => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    navigator.geolocation.getCurrentPosition.mockImplementation((success) => {
      success(mockPosition);
    });

    sbbApi.fetchNearbyStations.mockImplementation((lat, lon, callback) => {
      callback(null, mockStations);
    });

    // First call
    locationService.requestNearbyStations((err1, result1) => {
      // Second call immediately after
      locationService.requestNearbyStations((err2, result2) => {
        expect(result2).toEqual(mockStations);
        expect(navigator.geolocation.getCurrentPosition).toHaveBeenCalledTimes(1);
        expect(sbbApi.fetchNearbyStations).toHaveBeenCalledTimes(1);
        done();
      });
    });
  });

  test('requestNearbyStations re-fetches after cache expires', (done) => {
    const mockPosition = {
      coords: { latitude: 47.3769, longitude: 8.5417 }
    };

    const mockStations = [
      { id: '8503000', name: 'Zürich HB', distance: 1200 }
    ];

    navigator.geolocation.getCurrentPosition.mockImplementation((success) => {
      success(mockPosition);
    });

    sbbApi.fetchNearbyStations.mockImplementation((lat, lon, callback) => {
      callback(null, mockStations);
    });

    // First call
    locationService.requestNearbyStations((err1, result1) => {
      // Mock time passing
      locationService._expireCache();

      // Second call after cache expired
      locationService.requestNearbyStations((err2, result2) => {
        expect(navigator.geolocation.getCurrentPosition).toHaveBeenCalledTimes(2);
        expect(sbbApi.fetchNearbyStations).toHaveBeenCalledTimes(2);
        done();
      });
    });
  });

  test('requestNearbyStations returns cached data on GPS error', (done) => {
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
    sbbApi.fetchNearbyStations.mockImplementation((lat, lon, callback) => {
      callback(null, mockStations);
    });

    locationService.requestNearbyStations((err1, result1) => {
      // Clear cache to force GPS call
      locationService._expireCache();

      // Second call with GPS error
      navigator.geolocation.getCurrentPosition.mockImplementationOnce((success, error) => {
        error({ code: 1, message: 'User denied' });
      });

      locationService.requestNearbyStations((err2, result2) => {
        expect(result2).toEqual(mockStations);
        done();
      });
    });
  });

  test('requestNearbyStations throws error when no cache and GPS fails', (done) => {
    navigator.geolocation.getCurrentPosition.mockImplementation((success, error) => {
      error({ code: 1, message: 'User denied' });
    });

    locationService.requestNearbyStations((err, result) => {
      expect(err).toBeTruthy();
      expect(result).toBeNull();
      done();
    });
  });
});
