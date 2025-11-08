const sbbApi = require('../src/pkjs/sbb_api');

// Mock global fetch
global.fetch = jest.fn();

describe('SBB API Client', () => {
  beforeEach(() => {
    fetch.mockClear();
  });

  test('fetchNearbyStations returns stations sorted by distance', (done) => {
    const mockResponse = {
      stations: [
        { id: '8503000', name: 'Zürich HB', distance: 1200 },
        { id: '8503006', name: 'Zürich Stadelhofen', distance: 800 },
        { id: '8503020', name: 'Zürich Hardbrücke', distance: 2000 }
      ]
    };

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse
    });

    sbbApi.fetchNearbyStations(47.3769, 8.5417, (err, result) => {
      expect(err).toBeNull();
      expect(result).toHaveLength(3);
      expect(result[0].id).toBe('8503000');
      expect(result[0].name).toBe('Zürich HB');
      expect(result[0].distance).toBe(1200);
      expect(fetch).toHaveBeenCalledWith(
        expect.stringContaining('x=8.5417&y=47.3769')
      );
      done();
    });
  });

  test('fetchNearbyStations limits to 10 stations', (done) => {
    const mockStations = Array(20).fill(null).map((_, i) => ({
      id: `${8503000 + i}`,
      name: `Station ${i}`,
      distance: i * 100
    }));

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ stations: mockStations })
    });

    sbbApi.fetchNearbyStations(47.3769, 8.5417, (err, result) => {
      expect(err).toBeNull();
      expect(result).toHaveLength(10);
      done();
    });
  });

  test('fetchNearbyStations handles fetch error', (done) => {
    fetch.mockRejectedValueOnce(new Error('Network error'));

    sbbApi.fetchNearbyStations(47.3769, 8.5417, (err, result) => {
      expect(err).toBeTruthy();
      expect(err.message).toBe('Network error');
      expect(result).toBeNull();
      done();
    });
  });

  test('fetchNearbyStations handles HTTP 404 error', (done) => {
    fetch.mockRejectedValueOnce(new Error('HTTP error! status: 404'));

    sbbApi.fetchNearbyStations(47.3769, 8.5417, (err, result) => {
      expect(err).toBeTruthy();
      expect(result).toBeNull();
      done();
    });
  });

  test('fetchConnections returns connection array', (done) => {
    const mockResponse = {
      connections: [
        {
          from: {
            departure: '2025-11-07T14:32:00+0100',
            delay: 0
          },
          to: {
            arrival: '2025-11-07T15:47:00+0100'
          },
          sections: [
            {
              departure: {
                station: { name: 'Zürich HB' },
                departure: '2025-11-07T14:32:00+0100',
                platform: '7',
                delay: 0
              },
              arrival: {
                station: { name: 'Bern' },
                arrival: '2025-11-07T15:47:00+0100'
              },
              journey: {
                category: 'IC',
                number: '712'
              }
            }
          ]
        }
      ]
    };

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse
    });

    sbbApi.fetchConnections('8503000', '8507000', (err, result) => {
      expect(err).toBeNull();
      expect(result).toHaveLength(1);
      expect(result[0].sections).toHaveLength(1);
      expect(result[0].sections[0].trainType).toBe('IC 712');
      expect(result[0].sections[0].platform).toBe('7');
      expect(result[0].numChanges).toBe(0);
      done();
    });
  });

  test('fetchConnections calculates number of changes correctly', (done) => {
    const mockResponse = {
      connections: [
        {
          from: { departure: '2025-11-07T14:32:00+0100', delay: 0 },
          to: { arrival: '2025-11-07T16:02:00+0100' },
          sections: [
            { /* section 1 */ },
            { /* section 2 */ },
            { /* section 3 */ }
          ]
        }
      ]
    };

    // Mock full sections data
    mockResponse.connections[0].sections = mockResponse.connections[0].sections.map((_, i) => ({
      departure: {
        station: { name: `Station ${i}` },
        departure: '2025-11-07T14:32:00+0100',
        platform: '1',
        delay: 0
      },
      arrival: {
        station: { name: `Station ${i + 1}` },
        arrival: '2025-11-07T15:00:00+0100'
      },
      journey: { category: 'IC', number: '1' }
    }));

    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => mockResponse
    });

    sbbApi.fetchConnections('8503000', '8508500', (err, result) => {
      expect(err).toBeNull();
      expect(result[0].numChanges).toBe(2); // 3 sections = 2 changes
      done();
    });
  });

  test('fetchConnections handles HTTP 500 error', (done) => {
    fetch.mockRejectedValueOnce(new Error('HTTP error! status: 500'));

    sbbApi.fetchConnections('8503000', '8507000', (err, result) => {
      expect(err).toBeTruthy();
      expect(result).toBeNull();
      done();
    });
  });
});
