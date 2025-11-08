var sbbApi = require('./sbb_api');

var lastKnownStations = [];
var lastGPSTime = 0;
var GPS_CACHE_DURATION = 10 * 60 * 1000; // 10 minutes

function requestNearbyStations(callback) {
    var now = Date.now();

    // Return cached stations if recent
    if (lastKnownStations.length > 0 && (now - lastGPSTime) < GPS_CACHE_DURATION) {
        console.log('Using cached nearby stations');
        callback(null, lastKnownStations);
        return;
    }

    // Request GPS location
    navigator.geolocation.getCurrentPosition(
        function(position) {
            var lat = position.coords.latitude;
            var lon = position.coords.longitude;
            console.log('GPS: ' + lat + ', ' + lon);

            sbbApi.fetchNearbyStations(lat, lon, function(err, stations) {
                if (err) {
                    // If error but have cached data, return cached
                    if (lastKnownStations.length > 0) {
                        callback(null, lastKnownStations);
                    } else {
                        callback(err, null);
                    }
                } else {
                    lastKnownStations = stations;
                    lastGPSTime = now;
                    callback(null, stations);
                }
            });
        },
        function(error) {
            console.error('GPS error:', error);
            // Return cached stations if available
            if (lastKnownStations.length > 0) {
                callback(null, lastKnownStations);
            } else {
                callback(error, null);
            }
        },
        {
            timeout: 10000,
            maximumAge: GPS_CACHE_DURATION,
            enableHighAccuracy: false
        }
    );
}

module.exports = {
    requestNearbyStations: requestNearbyStations
};
