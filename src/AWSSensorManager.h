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

#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include "Adafruit_TSL2591.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>

#include "AWSGPS.h"
#include "AWSConfig.h"
#include "SQM.h"
#include "Hydreon.h"
#include "AWSWind.h"

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



class AWSSensorManager {

  private:

    Adafruit_BME280		*bme;
    Adafruit_MLX90614	*mlx;
    Adafruit_TSL2591	*tsl;
    Hydreon				*rain_sensor;
    SQM					*sqm;
    AWSGPS				*gps;
	AWSWindSensor		*wind_sensors;

	AWSConfig 			*config;
	
    char 				hw_version[6];
    uint8_t 			available_sensors;
    sensor_data_t		sensor_data;
    bool				debug_mode,
						rain_event,
						solar_panel;
    TaskHandle_t		sensors_task_handle;
    SemaphoreHandle_t	i2c_mutex = NULL;
   	uint32_t			polling_ms_interval;

  public:
    					AWSSensorManager( bool, bool );
    bool				begin( void );
    uint8_t				get_available_sensors( void );
    bool				get_debug_mode( void );
    SemaphoreHandle_t	get_i2c_mutex( void );
    sensor_data_t		*get_sensor_data( void );
    bool				initialise( I2C_SC16IS750 *, AWSConfig *, bool );
    bool				initialise_rain_sensor( void );
    void				initialise_sensors( I2C_SC16IS750 * );
    bool				poll_sensors( void );
    const char 			*rain_intensity_str( void );
    bool				rain_sensor_available( void );
    void 				read_battery_level( void );
    void				read_rain_sensor( void );
    void				read_sensors( void );
    void				reset_rain_event( void );
    void				set_rain_event( void );

  private:

bool sync_time( void );

    void initialise_BME( void );
    void initialise_GPS( I2C_SC16IS750 * );
    void initialise_MLX( void );
    void initialise_TSL( void );
    void poll_sensors_task( void * );
    void read_anemometer();
    void read_BME( void );
    void read_GPS( void );
    void read_MLX( void );
    void read_TSL( void );
    void read_wind_vane( void );
    void retrieve_sensor_data( void );

};

#endif
