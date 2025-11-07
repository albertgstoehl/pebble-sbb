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
    ).rejects.toBeTruthy();
  });
});
