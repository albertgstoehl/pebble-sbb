var sbbApi = require('./sbb_api');
var locationService = require('./location_service');

function handleAppMessage(event) {
    var message = event.payload;
    console.log('Received message:', JSON.stringify(message));

    if (message.REQUEST_NEARBY_STATIONS !== undefined) {
        handleNearbyStationsRequest();
    } else if (message.REQUEST_CONNECTIONS !== undefined) {
        handleConnectionsRequest(
            message.DEPARTURE_STATION_ID,
            message.ARRIVAL_STATION_ID
        );
    }
}

function handleNearbyStationsRequest() {
    locationService.requestNearbyStations(function(err, stations) {
        if (err) {
            sendError('GPS unavailable. Check location settings.');
            return;
        }

        // Check if no stations were found
        if (!stations || stations.length === 0) {
            sendError('No stations found nearby. Try favorites.');
            return;
        }

        // Send stations one at a time to avoid message size limits
        stations.forEach(function(station, index) {
            Pebble.sendAppMessage({
                STATION_ID: station.id,
                STATION_NAME: station.name,
                STATION_DISTANCE: station.distance
            }, function() {
                console.log('Sent station ' + (index + 1) + '/' + stations.length);
            }, function() {
                console.error('Failed to send station ' + station.name);
            });
        });
    });
}

function handleConnectionsRequest(fromId, toId) {
    if (!fromId || !toId) {
        sendError('Invalid station IDs');
        return;
    }

    sbbApi.fetchConnections(fromId, toId, function(err, connections) {
        if (err) {
            sendError('Network error. Check connection.');
            return;
        }

        if (connections.length === 0) {
            sendError('No connections found');
            return;
        }

        // Send first connection (we'll enhance this later for multiple)
        var conn = connections[0];
        Pebble.sendAppMessage({
            CONNECTION_DATA: 1,
            DEPARTURE_TIME: conn.departureTime,
            ARRIVAL_TIME: conn.arrivalTime,
            PLATFORM: conn.sections[0].platform,
            TRAIN_TYPE: conn.sections[0].trainType,
            DELAY_MINUTES: conn.totalDelayMinutes,
            NUM_CHANGES: conn.numChanges
        }, function() {
            console.log('Sent connection data');
        }, function() {
            console.error('Failed to send connection data');
        });
    });
}

function sendError(message) {
    Pebble.sendAppMessage({
        ERROR_MESSAGE: message
    }, function() {
        console.log('Sent error:', message);
    }, function() {
        console.error('Failed to send error message');
    });
}

module.exports = {
    handleAppMessage: handleAppMessage
};
