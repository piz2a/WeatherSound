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
    const coverage_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=cloud_cover`
    const humidity_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=relative_humidity_2m`
    const temp_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=temperature_2m`
    const uv_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=uv_index`
    const wspeed_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=wind_speed_10m`
    const wdir_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=wind_direction_10m`
    const vis_url = `https://api.open-meteo.com/v1/forecast?latitude=${latitude}&longitude=${longitude}&current=visibility`

    const getJSON = async (url: string) => {
      const resp = await fetch(url);
      if (!resp.ok) {
        throw new Error(`Response status: ${resp.status}`);
      }
      return await resp.json()
    }

    return {
      cloudCoverage: clamp((await getJSON(coverage_url)).current.cloud_cover, 0, 100), // %
      humidity: clamp((await getJSON(humidity_url)).current.relative_humidity_2m, 0, 100), // %
      temperature: clamp((await getJSON(temp_url)).current.temperature_2m, -40, 50), // C -40 to +50 CLAMPED
      uvIndex: clamp((await getJSON(uv_url)).current.uv_index, 0, 11), // 0 to 11
      windSpeed: clamp((await getJSON(wspeed_url)).current.wind_speed_10m, 0, 100), // 0 to 100
      windDirection: clamp((await getJSON(wdir_url)).current.wind_direction_10m, 0, 360), // 0 - 360
      visibility: clamp((await getJSON(vis_url)).current.visibility, 0, 296000) // 0 to 296000
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