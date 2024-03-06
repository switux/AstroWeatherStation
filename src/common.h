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

#include "build_id.h"
#include "AWSGPS.h"

// Force DEBUG output even if not activated by external button
const byte DEBUG_MODE = 1;

extern const unsigned long 		US_SLEEP;
extern const etl::string<12>	REV;
extern HardwareSerial			Serial1;


enum class aws_device_t : unsigned long {
	NO_SENSOR			= 0x00000000,
	MLX_SENSOR			= 0x00000001,
	TSL_SENSOR			= 0x00000002,
	BME_SENSOR			= 0x00000004,
	WIND_VANE_SENSOR	= 0x00000008,
	ANEMOMETER_SENSOR	= 0x00000010,
	RAIN_SENSOR			= 0x00000020,
	GPS_SENSOR			= 0x00000040,
	DOME_DEVICE			= 0x00000080,
	ETHERNET_DEVICE		= 0x00000100,
	SC16IS750_DEVICE	= 0x00000200
};

extern aws_device_t	operator&( aws_device_t, aws_device_t );
extern bool			operator!=( unsigned long, aws_device_t );
extern aws_device_t operator&( unsigned long, aws_device_t );
extern aws_device_t operator~( aws_device_t );
extern aws_device_t operator|( aws_device_t, aws_device_t );
extern aws_device_t operator*( aws_device_t, int );
extern aws_device_t &operator|=( aws_device_t &, aws_device_t );
extern aws_device_t &operator&=( aws_device_t &, aws_device_t );

extern const aws_device_t ALL_SENSORS;

enum struct dome_shutter_status_type : byte
{
	Open,
	Closed,
	Opening,
	Closing
};
using dome_shutter_status_t = dome_shutter_status_type;

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
	float	raw_sky_temperature;
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
	aws_device_t	available_sensors;

};

struct dome_data_t {
	
	dome_shutter_status_t	shutter_status;
	bool					closed_sensor;
	bool					moving_sensor;
	bool					close_command;
};

struct station_data_t {

	gps_data_t		gps;
	health_data_t	health;
	dome_data_t		dome_data;
//	lookout_data_t	lookout_data;
	struct timeval	ntp_time;
	int				reset_reason0;
	int				reset_reason1;
};

void loop( void );
void setup( void );

template <typename T>
int sign( T val )
{
	return static_cast<int>(T(0) < val) - static_cast<int>(val < T(0));
}

#endif // _common_H
