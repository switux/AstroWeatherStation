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

#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0

#define DEBUG_MODE 1

#define REV "3.0.0.0"
#define BUILD_DATE "20231119"

// Misc
#define GPIO_DEBUG		GPIO_NUM_34

//#define	CONFIG_MODE_GUARD	5000000						// 5 seconds
#define	CONFIG_MODE_GUARD	1000000						// 5 seconds
#define US_SLEEP		5 * 60 * 1000000				// 5 minutes
#define US_HIBERNATE	30 * 24 * 60 * 60 * 1000000ULL	// 30 days

#define	USE_MLX				"1"
#define	USE_BME				"1"
#define	USE_TSL				"1"
#define	USE_WV				"1"
#define	USE_WS				"1"
#define	USE_RG9				"1"
#define	USE_GPS				"0"

// DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING!

#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_       0

#define	SPI_CLOCK_MHZ		18

#define MLX_SENSOR			0x00000001
#define TSL_SENSOR			0x00000002
#define BME_SENSOR			0x00000004 // 0000 0000 0000 0000 0000 0000 0000 0100
#define WIND_VANE_SENSOR	0x00000008 // 0000 0000 0000 0000 0000 0000 0000 1000
#define ANEMOMETER_SENSOR	0x00000010 // 0000 0000 0000 0000 0000 0000 0001 0000
#define RG9_SENSOR			0x00000020 // 0000 0000 0000 0000 0000 0000 0010 0000
#define	GPS_SENSOR			0x00000040 // 0000 0000 0000 0000 0000 0000 0100 0000
#define ALL_SENSORS			( MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RG9_SENSOR | GPS_SENSOR )

#define FORMAT_SPIFFS_IF_FAILED true

#define	ASCOM_RG9_DESCRIPTION		"Hydreon RG-9 Rain sensor"
#define	ASCOM_RG9_DRIVER_INFO		"Hydreon RG-9 Driver (c) OpenAstroDevices 2023"
#define	ASCOM_RG9_DRIVER_VERSION	"1.0.1.0"
#define	ASCOM_RG9_NAME				"Hydreon RG-9"

#define	ASCOM_DOME_DESCRIPTION		"Generic roof top"
#define	ASCOM_DOME_DRIVER_INFO		"Generic roof top driver (c) OpenAstroDevices 2023"
#define	ASCOM_DOME_DRIVER_VERSION	"1.0.0.0"
#define	ASCOM_DOMEffff_NAME				"Generic roof top"

struct sensor_data_t {

	char			*ota_board;
	char			*ota_device;
	char			*ota_config;

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
