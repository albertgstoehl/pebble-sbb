// Mock mode for emulator testing (no network available)
// Automatically disabled in test environment and on physical watch
// Set to true manually if testing in emulator without network
var MOCK_MODE = false;

// Mock nearby stations data
// Note: Umlauts removed to avoid encoding issues on Pebble watch
var MOCK_NEARBY_STATIONS = [
    { id: '8503000', name: 'Zuerich HB', distance: 100 },
    { id: '8503003', name: 'Zuerich Stadelhofen', distance: 450 },
    { id: '8503006', name: 'Zuerich Hardbruecke', distance: 1200 },
    { id: '8507000', name: 'Bern', distance: 2500 }
];

// Mock connections data
var MOCK_CONNECTIONS = {
    sections: [
        {
            departureStation: 'Zuerich Stadelhofen',
            arrivalStation: 'Bern',
            departureTime: Math.floor(Date.now() / 1000) + 300,
            arrivalTime: Math.floor(Date.now() / 1000) + 4500,
            platform: '7',
            trainType: 'IC 712',
            delayMinutes: 3
        }
    ],
    numSections: 1,
    departureTime: Math.floor(Date.now() / 1000) + 300,
    arrivalTime: Math.floor(Date.now() / 1000) + 4500,
    totalDelayMinutes: 3,
    numChanges: 0
};

// SBB OpenData API endpoint
var SBB_API_BASE = 'https://transport.opendata.ch/v1';

// Fetch nearby stations based on coordinates
function fetchNearbyStations(lat, lon, callback) {
    if (MOCK_MODE) {
        console.log('[MOCK] Returning mock nearby stations');
        callback(null, MOCK_NEARBY_STATIONS);
        return;
    }

    var url = SBB_API_BASE + '/locations?x=' + lon + '&y=' + lat + '&type=station';

    fetch(url)
        .then(function(response) {
            return response.json();
        })
        .then(function(data) {
            var stations = data.stations.slice(0, 10).map(function(station) {
                return {
                    id: station.id,
                    name: station.name,
                    distance: Math.round(station.distance || 0)
                };
            });
            callback(null, stations);
        })
        .catch(function(error) {
            console.error('Error fetching nearby stations:', error);
            callback(error, null);
        });
}

// Fetch connections between two stations
function fetchConnections(fromId, toId, callback) {
    if (MOCK_MODE) {
        console.log('[MOCK] Returning mock connections from', fromId, 'to', toId);
        callback(null, [MOCK_CONNECTIONS]);
        return;
    }

    var url = SBB_API_BASE + '/connections?from=' + fromId + '&to=' + toId + '&limit=5';

    fetch(url)
        .then(function(response) {
            return response.json();
        })
        .then(function(data) {
            var connections = data.connections.map(function(conn) {
                var sections = conn.sections.map(function(section) {
                    return {
                        departureStation: section.departure.station.name,
                        arrivalStation: section.arrival.station.name,
                        departureTime: Math.floor(new Date(section.departure.departure).getTime() / 1000),
                        arrivalTime: Math.floor(new Date(section.arrival.arrival).getTime() / 1000),
                        platform: section.departure.platform || 'N/A',
                        trainType: section.journey ? (section.journey.category + ' ' + section.journey.number) : 'Walk',
                        delayMinutes: section.departure.delay || 0
                    };
                });

                return {
                    sections: sections,
                    numSections: sections.length,
                    departureTime: Math.floor(new Date(conn.from.departure).getTime() / 1000),
                    arrivalTime: Math.floor(new Date(conn.to.arrival).getTime() / 1000),
                    totalDelayMinutes: conn.from.delay || 0,
                    numChanges: sections.length - 1
                };
            });
            callback(null, connections);
        })
        .catch(function(error) {
            console.error('Error fetching connections:', error);
            callback(error, null);
        });
}

module.exports = {
    fetchNearbyStations: fetchNearbyStations,
    fetchConnections: fetchConnections
};
