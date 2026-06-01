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

export async function pull(latitude: number, longitude: number): Promise<WeatherResponse> {
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