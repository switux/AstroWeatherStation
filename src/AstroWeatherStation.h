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

#define DEBUG_MODE 1

#define REV "3.0.0.0"
#define BUILD_DATE "20230902"

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

#define	_STA_SSID			0x00000080 // 0000 0000 0000 0000 0000 0000 1000 0000
#define	_WIFI_STA_PASSWORD	0x00000100 // 0000 0000 0000 0000 0000 0001 0000 0000
#define	_TZNAME				0x00000200 // 0000 0000 0000 0000 0000 0010 0000 0000
#define	_REMOTE_SERVER		0x00000400 // 0000 0000 0000 0000 0000 0100 0000 0000
#define	_ROOT_CA			0x00000800 // 0000 0000 0000 0000 0000 1000 0000 0000
#define	_MSAS_CALIBRATION	0x00001000 // 0000 0000 0000 0000 0001 0000 0000 0000
#define	_CONFIG_PORT		0x00002000 // 0000 0000 0000 0000 0010 0000 0000 0000
#define	_URL_PATH			0x00004000 // 0000 0000 0000 0000 0100 0000 0000 0000
#define	_WIFI_MODE			0x00008000 // 0000 0000 0000 0000 1000 0000 0000 0000
#define	_PREF_IFACE			0x00010000 // 0000 0000 0000 0001 0000 0000 0000 0000
#define	_WIFI_STA_IP_MODE	0x00020000 // 0000 0000 0000 0010 0000 0000 0000 0000
#define	_WIFI_STA_IP		0x00040000 // 0000 0000 0000 0100 0000 0000 0000 0000
#define	_WIFI_STA_GW		0x00080000 // 0000 0000 0000 1000 0000 0000 0000 0000
#define	_WIFI_AP_IP_MODE	0x00100000 // 0000 0000 0001 0000 0000 0000 0000 0000
#define	_WIFI_AP_IP			0x00200000 // 0000 0000 0010 0000 0000 0000 0000 0000
#define	_WIFI_AP_GW			0x00400000 // 0000 0000 0100 0000 0000 0000 0000 0000
#define	_AP_SSID			0x00800000 // 0000 0000 1000 0000 0000 0000 0000 0000
#define	_WIFI_AP_PASSWORD	0x01000000 // 0000 0001 0000 0000 0000 0000 0000 0000

#define FORMAT_SPIFFS_IF_FAILED true

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
	int				wind_direction;
	float			dew_point;

	bool			rain_event;
	uint8_t			rain_intensity;
	
	float			ambient_temperature;
	float			sky_temperature;

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

/*
void deep_sleep( int );
void enter_config_mode( void );
void hibernate();
void reboot( void );
void reset_config_parameter( void );
const char *save_configuration( const char * );
void set_configuration( void );
void web_server_not_found( void );
*/


#endif // _AstroWeatherStation_H
