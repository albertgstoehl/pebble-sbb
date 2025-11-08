// SBB OpenData API endpoint
var SBB_API_BASE = 'https://transport.opendata.ch/v1';

// Fetch nearby stations based on coordinates
function fetchNearbyStations(lat, lon, callback) {
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
