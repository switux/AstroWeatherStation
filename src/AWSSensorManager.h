/*
  	AWSSensorManager.h

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
#ifndef _AWSSensorManager_H
#define _AWSSensorManager_H

#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0

#include <SoftwareSerial.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include "Adafruit_TSL2591.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>


#include "AWSConfig.h"

#define SEND	HIGH
#define RECV	LOW

#define WIND_VANE_MAX_TRIES		3		// Tests show that if there is no valid answer after 3 attempts, the sensor is not available
#define ANEMOMETER_MAX_TRIES	3		// Tests show that if there is no valid answer after 3 attempts, the sensor is not available

#define RG9_SERIAL_SPEEDS	7
#define RG9_PROBE_RETRIES	3
#define RG9_OK				0
#define RG9_FAIL			127
#define RG9_UART			2

#define	GPS_SPEED		9600

#define LOW_BATTERY_COUNT_MIN	5
#define LOW_BATTERY_COUNT_MAX	10
#define BAT_V_MAX				4200	// in mV
#define BAT_V_MIN				3000	// in mV
#define BAT_LEVEL_MIN			33		// in %, corresponds to ~3.4V for a typical Li-ion battery
#define VCC						3300	// in mV
#define V_DIV_R1				82000	// voltage divider R1 in ohms
#define V_DIV_R2				300000	// voltage divider R2 in ohms
#define ADC_MAX					4096	// 12 bits resolution
#define V_MAX_IN				( BAT_V_MAX*V_DIV_R2 )/( V_DIV_R1+V_DIV_R2 )	// in mV
#define V_MIN_IN				( BAT_V_MIN*V_DIV_R2 )/( V_DIV_R1+V_DIV_R2 )	// in mV
#define ADC_V_MAX				( V_MAX_IN*ADC_MAX / VCC )
#define ADC_V_MIN				( V_MIN_IN*ADC_MAX / VCC )

#define	LUX_TO_IRRADIANCE_FACTOR	0.88
#define	TSL_MAX_LUX					88000

struct anemometer_t {

  SoftwareSerial	*device;
  uint8_t			request_cmd[8];
  byte			byte_idx;
};

struct wind_vane_t {

  SoftwareSerial	*device;
  uint8_t			request_cmd[8];
  byte			byte_idx;
};


class AWSSensorManager {

  private:

    Adafruit_BME280		*bme;
    Adafruit_MLX90614	*mlx;
    Adafruit_TSL2591	*tsl;
    HardwareSerial		*rg9;
    SQM					*sqm;
    AWSGPS				*gps;
    wind_vane_t			wind_vane;
    anemometer_t		anemometer;
    AWSConfig 			*config;
    char 				hw_version[6];
    uint8_t 			available_sensors;
    sensor_data_t		sensor_data;
    bool				debug_mode,
    					rg9_initialised,
						rain_event,
						solar_panel;
    TaskHandle_t		sensors_task_handle;
    SemaphoreHandle_t	i2c_mutex = NULL;
    /*
      #ifdef USE_SC16IS750
    		SC16IS750			*sc16is750;
      #endif
    */
  public:
    AWSSensorManager( void );
    bool			begin( void );
    uint8_t			get_available_sensors( void );
    bool			get_debug_mode( void );
    SemaphoreHandle_t	get_i2c_mutex( void );
    sensor_data_t	*get_sensor_data( void );
    bool			initialise( I2C_SC16IS750 *, AWSConfig *, bool );
    bool			initialise_RG9( void );
    void			initialise_sensors( I2C_SC16IS750 * );
    void			read_RG9( void );
    void			read_sensors( void );
    void			reset_rain_event( void );
    void			set_rain_event( void );


  private:
    bool sync_time( void );
    void read_battery_level( void );

    void initialise_anemometer( void );
    void initialise_BME( void );
    void initialise_GPS( I2C_SC16IS750 * );
    void initialise_MLX( void );
    void initialise_TSL( void );
    void initialise_wind_vane( void );
    void poll_sensors_task( void * );
    void read_anemometer( void );
    void read_BME( void );
    void read_GPS( void );
    void read_MLX( void );
    void read_TSL( void );
    void read_wind_vane( void );
    void retrieve_sensor_data( void );
    uint8_t RG9_read_string( char *, uint8_t );
    const char *RG9_reset_cause( char );
    void uint64_t_to_uint8_t_array( uint64_t, uint8_t * );


};

#endif
