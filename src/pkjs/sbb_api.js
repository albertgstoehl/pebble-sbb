// SBB OpenData API endpoint
const SBB_API_BASE = 'https://transport.opendata.ch/v1';

// Fetch nearby stations based on coordinates
async function fetchNearbyStations(lat, lon) {
  const url = `${SBB_API_BASE}/locations?x=${lon}&y=${lat}&type=station`;

  const response = await fetch(url);
  const data = await response.json();

  const stations = data.stations.slice(0, 10).map(station => ({
    id: station.id,
    name: station.name,
    distance: Math.round(station.distance || 0)
  }));

  return stations;
}

// Fetch connections between two stations
async function fetchConnections(fromId, toId) {
  const url = `${SBB_API_BASE}/connections?from=${fromId}&to=${toId}&limit=5`;

  const response = await fetch(url);
  const data = await response.json();

  const connections = data.connections.map(conn => {
    const sections = conn.sections.map(section => ({
      departureStation: section.departure.station.name,
      arrivalStation: section.arrival.station.name,
      departureTime: Math.floor(new Date(section.departure.departure).getTime() / 1000),
      arrivalTime: Math.floor(new Date(section.arrival.arrival).getTime() / 1000),
      platform: section.departure.platform || 'N/A',
      trainType: section.journey ? `${section.journey.category} ${section.journey.number}` : 'Walk',
      delayMinutes: section.departure.delay || 0
    }));

    return {
      sections: sections,
      numSections: sections.length,
      departureTime: Math.floor(new Date(conn.from.departure).getTime() / 1000),
      arrivalTime: Math.floor(new Date(conn.to.arrival).getTime() / 1000),
      totalDelayMinutes: conn.from.delay || 0,
      numChanges: sections.length - 1
    };
  });

  return connections;
}

module.exports = {
  fetchNearbyStations,
  fetchConnections
};
