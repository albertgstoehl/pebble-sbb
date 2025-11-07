const sbbApi = require('./sbb_api');
const locationService = require('./location_service');

async function handleAppMessage(event) {
  const message = event.payload;

  if (message.REQUEST_NEARBY_STATIONS !== undefined) {
    await handleNearbyStationsRequest();
  } else if (message.REQUEST_CONNECTIONS !== undefined) {
    await handleConnectionsRequest(
      message.DEPARTURE_STATION_ID,
      message.ARRIVAL_STATION_ID
    );
  }
}

async function handleNearbyStationsRequest() {
  try {
    const stations = await locationService.requestNearbyStations();

    // Send stations one at a time
    for (const station of stations) {
      await sendMessage({
        STATION_ID: station.id,
        STATION_NAME: station.name,
        STATION_DISTANCE: station.distance
      });
    }
  } catch (err) {
    sendError('GPS unavailable. Check location settings.');
  }
}

async function handleConnectionsRequest(fromId, toId) {
  if (!fromId || !toId) {
    sendError('Invalid station IDs');
    return;
  }

  try {
    const connections = await sbbApi.fetchConnections(fromId, toId);

    if (connections.length === 0) {
      sendError('No connections found');
      return;
    }

    // Send first connection
    const conn = connections[0];
    await sendMessage({
      CONNECTION_DATA: 1,
      DEPARTURE_TIME: conn.departureTime,
      ARRIVAL_TIME: conn.arrivalTime,
      PLATFORM: conn.sections[0].platform,
      TRAIN_TYPE: conn.sections[0].trainType,
      DELAY_MINUTES: conn.totalDelayMinutes,
      NUM_CHANGES: conn.numChanges
    });
  } catch (err) {
    sendError('Network error. Check connection.');
  }
}

function sendMessage(message) {
  return new Promise((resolve, reject) => {
    Pebble.sendAppMessage(
      message,
      () => resolve(),
      () => reject(new Error('Failed to send message'))
    );
  });
}

function sendError(message) {
  sendMessage({ ERROR_MESSAGE: message });
}

module.exports = {
  handleAppMessage
};
