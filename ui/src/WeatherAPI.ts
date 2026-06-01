export type WeatherResponse = {
  cloudCoverage: number,
  humidity: number,
  temperature: number,
  uvIndex: number,
  windSpeed: number,
  windDirection: number,
  visibility: number
}

function clamp(v: number, l: number, u: number) {
  return v >= u ? u : v <= l ? l : v
}

export async function pull(locationIndex: number): Promise<WeatherResponse> {
  const getCoordinates = (locationIndex: number): { lat: number; lon: number } => {
  switch (locationIndex) {
    case 0: // Bogota
      return { lat: 4.7110, lon: -74.0721 };
    case 1: // Seoul
      return { lat: 37.5665, lon: 126.9780 };
    case 2: // Kalamazoo
      return { lat: 42.2917, lon: -85.5872 };
    case 3: // Tokyo
      return { lat: 35.6764, lon: 139.6500 };
    case 4: // Ushuaia
      return { lat: -54.8019, lon: -68.3030 };
    case 5: // Cape Town
      return { lat: -33.9249, lon: 18.4241 };
    case 6: // Austin
      return { lat: 30.2672, lon: -97.7431 };
    case 7: // Dubai
      return { lat: 25.2048, lon: 55.2708 };
    default:
      // Fallback for out-of-bound indices
      return { lat: 0.0, lon: 0.0 };
  }
};
const coords = getCoordinates(locationIndex)
const latitude = coords.lat
const longitude = coords.lon
  try {
    const url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=cloud_cover,relative_humidity_2m,temperature_2m,uv_index,wind_speed_10m,wind_direction_10m,visibility`;
    const resp = await fetch(url);

    if (!resp.ok) {
      throw new Error(`Response status: ${resp.status}`);
    }

    const data = await resp.json();
    const current = data.current;

    return {
      cloudCoverage: clamp(current.cloud_cover, 0, 100), // %
      humidity: clamp(current.relative_humidity_2m, 0, 100), // %
      temperature: clamp(current.temperature_2m, -40, 50), // C -40 to +50 CLAMPED
      uvIndex: clamp(current.uv_index, 0, 11), // 0 to 11
      windSpeed: clamp(current.wind_speed_10m, 0, 100), // 0 to 100
      windDirection: clamp(current.wind_direction_10m, 0, 360), // 0 - 360
      visibility: clamp(current.visibility, 0, 296000) // 0 to 296000
    }
  } catch (error) {
    console.error((error as Error).message);
    return {
      cloudCoverage: 0,
      humidity: 0,
      temperature: 0,
      uvIndex: 0,
      windSpeed: 0,
      windDirection: 0,
      visibility: 0
    }
  }
}