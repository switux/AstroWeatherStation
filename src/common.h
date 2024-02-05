/*
	AstroWeatherStation.h

	(c) 2023-2024 F.Lesage

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
#ifndef _common_H
#define _common_H

#include "build_seq.h"

// Force DEBUG output even if not activated by external button
const byte DEBUG_MODE = 1;

extern const unsigned long 	US_SLEEP;
extern const char			*REV;
extern HardwareSerial		Serial1;

// ------------------------------------------------------------------------------
//
// DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING!
//
// ------------------------------------------------------------------------------

#include "AWSGPS.h"

struct aws_health_data_t {

	float			battery_level;
	uint32_t		uptime;
	uint32_t		init_heap_size;
	uint32_t		current_heap_size;
	uint32_t		largest_free_heap_block;
};

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
	byte			rain_intensity;

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
