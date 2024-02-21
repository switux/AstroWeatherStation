/*
	common.h

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

#include "Embedded_Template_Library.h"
#include "etl/string.h"
#include "etl/string_utilities.h"

#include "build_seq.h"
#include "AWSGPS.h"

// Force DEBUG output even if not activated by external button
const byte DEBUG_MODE = 1;

extern const unsigned long 		US_SLEEP;
extern const etl::string<12>	REV;
extern HardwareSerial			Serial1;

struct health_data_t {

	float			battery_level;
	uint32_t		uptime;
	uint32_t		init_heap_size;
	uint32_t		current_heap_size;
	uint32_t		largest_free_heap_block;

};

struct sqm_data_t {

	float		msas;
	float		nelm;
	uint16_t	gain;
	uint16_t	integration_time;
	uint16_t	ir_luminosity;
	uint16_t	vis_luminosity;
	uint16_t	full_luminosity;
	
};

struct weather_data_t {

	float	temperature;
	float	pressure;
	float	sl_pressure;
	float	rh;
	float	wind_speed;
	float	wind_gust;
	int		wind_direction;
	float	dew_point;

	bool	rain_event;
	byte	rain_intensity;

	float	ambient_temperature;
	float	sky_temperature;
	float	cloud_cover;
	byte	cloud_coverage;
	
};

struct sun_data_t {

	int		lux;
	float	irradiance;
	
};

struct sensor_data_t {

	time_t			timestamp;
	sun_data_t		sun;
	weather_data_t	weather;
	sqm_data_t		sqm;

};

struct station_data_t {

	gps_data_t		gps;
	health_data_t	health;
	struct timeval	ntp_time;

};

void loop( void );
void setup( void );

#endif // _common_H
