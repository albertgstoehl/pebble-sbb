const sbbApi = require('./sbb_api');

let lastKnownStations = [];
let lastGPSTime = 0;
const GPS_CACHE_DURATION = 10 * 60 * 1000; // 10 minutes

async function requestNearbyStations() {
  const now = Date.now();

  // Return cached stations if recent
  if (lastKnownStations.length > 0 && (now - lastGPSTime) < GPS_CACHE_DURATION) {
    return lastKnownStations;
  }

  // Request GPS location
  return new Promise((resolve, reject) => {
    navigator.geolocation.getCurrentPosition(
      async (position) => {
        const lat = position.coords.latitude;
        const lon = position.coords.longitude;

        try {
          const stations = await sbbApi.fetchNearbyStations(lat, lon);
          lastKnownStations = stations;
          lastGPSTime = now;
          resolve(stations);
        } catch (err) {
          // If error but have cached data, return cached
          if (lastKnownStations.length > 0) {
            resolve(lastKnownStations);
          } else {
            reject(err);
          }
        }
      },
      (error) => {
        // Return cached stations if available
        if (lastKnownStations.length > 0) {
          resolve(lastKnownStations);
        } else {
          reject(error);
        }
      },
      {
        timeout: 10000,
        maximumAge: GPS_CACHE_DURATION,
        enableHighAccuracy: false
      }
    );
  });
}

// Test helpers
function _clearCache() {
  lastKnownStations = [];
  lastGPSTime = 0;
}

function _expireCache() {
  lastGPSTime = 0;
}

module.exports = {
  requestNearbyStations,
  _clearCache,
  _expireCache
};
