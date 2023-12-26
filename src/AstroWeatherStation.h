/*
	AstroWeatherStation.h

	(c) 2023 F.Lesage

	This program is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with this program. If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once
#ifndef _AstroWeatherStation_H
#define _AstroWeatherStation_H

#define REV "3.0.0.0"
#define BUILD_DATE "2023122101"

// Force DEBUG output even if not activated by external button
#define DEBUG_MODE 1

// ------------------------------------------------------------------------------
//
// DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING!
//
// ------------------------------------------------------------------------------

#define	SPI_CLOCK_MHZ		18

#define MLX_SENSOR			0x00000001
#define TSL_SENSOR			0x00000002
#define BME_SENSOR			0x00000004
#define WIND_VANE_SENSOR	0x00000008
#define ANEMOMETER_SENSOR	0x00000010
#define RAIN_SENSOR			0x00000020
#define	GPS_SENSOR			0x00000040
#define ALL_SENSORS			( MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RAIN_SENSOR | GPS_SENSOR )

#define FORMAT_SPIFFS_IF_FAILED true

// Only applicable for solar panel version
#define	CONFIG_MODE_GUARD	5000000						// 5 seconds
#define US_SLEEP		5 * 60 * 1000000				// 5 minutes
#define US_HIBERNATE	1 * 24 * 60 * 60 * 1000000ULL	// 1 day

#include "AWSGPS.h"

struct sensor_data_t {

	time_t			timestamp;

	float			battery_level;

	float			temperature;
	float			pressure;
	float			sl_pressure;
	float			rh;
	float			wind_speed;
	float			wind_gust;
	int				wind_direction;
	float			dew_point;

	bool			rain_event;
	short			rain_intensity;

	float			ambient_temperature;
	float			sky_temperature;
	float			cloud_cover;
	
	float			msas;
	float			nelm;
	uint16_t		gain;
	uint16_t		integration_time;

	uint16_t		ir_luminosity;
	uint16_t		vis_luminosity;
	uint16_t		full_luminosity;
	int				lux;
	float			irradiance;

	struct timeval	ntp_time;

	gps_data_t		gps;
};

void loop( void );
void setup( void );

#endif // _AstroWeatherStation_H
