/*
  	sensor_manager.h

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
#ifndef _sensor_manager_H
#define _sensor_manager_H

#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include "Adafruit_TSL2591.h"
#include <Preferences.h>
#include <ArduinoJson.h>

#include "config_manager.h"
#include "SQM.h"
#include "device.h"
#include "Hydreon.h"
#include "anemometer.h"
#include "wind_vane.h"

const float			LUX_TO_IRRADIANCE_FACTOR	= 0.88;
const unsigned int	TSL_MAX_LUX					= 88000;

enum struct cloud_coverage : uint8_t {

	CLEAR,
	CLOUDY,
	OVERCAST

};

class AWSSensorManager {

  private:

    Adafruit_BME280		*bme				= nullptr;
    Adafruit_MLX90614	*mlx				= nullptr;
    Adafruit_TSL2591	*tsl				= nullptr;
    Hydreon				rain_sensor;
    SQM					sqm;
	Anemometer			anemometer;
	Wind_vane			wind_vane;
	std::array<int,7>	k;
	AWSConfig 			*config				= nullptr;
	SoftwareSerial		rs485_bus;

    aws_device_t		available_sensors	= aws_device_t::NO_SENSOR;
    sensor_data_t		sensor_data;
    bool				debug_mode			= false;
    bool				initialised			= false;
	bool				rain_event			= false;
	bool				solar_panel			= false;
    TaskHandle_t		sensors_task_handle;
    SemaphoreHandle_t	i2c_mutex			= nullptr;
   	uint32_t			polling_ms_interval	= DEFAULT_SENSOR_POLLING_MS_INTERVAL;

  public:
    					AWSSensorManager( void );
    bool				begin( void );
    aws_device_t		get_available_sensors( void );
    bool				get_debug_mode( void );
    SemaphoreHandle_t	get_i2c_mutex( void );
    sensor_data_t		*get_sensor_data( void );
    bool				initialise( I2C_SC16IS750 *, AWSConfig *, bool );
    bool				initialise_rain_sensor( void );
    void				initialise_sensors( I2C_SC16IS750 * );
    bool				poll_sensors( void );
    const char 			*rain_intensity_str( void );
    bool				rain_sensor_available( void );
    void				read_rain_sensor( void );
    void				read_sensors( void );
    void				reset_rain_event( void );
	void				resume( void );
	bool				sensor_is_available( aws_device_t );
    void				set_debug_mode( bool );
    void				set_rain_event( void );
	void				set_solar_panel( bool );
	void				suspend( void );

  private:

bool sync_time( void );

    void initialise_BME( void );
    void initialise_MLX( void );
    void initialise_TSL( void );
    void poll_sensors_task( void * );
    void read_anemometer();
    void read_BME( void );
    void read_MLX( void );
    void read_TSL( void );
    void read_wind_vane( void );
    void retrieve_sensor_data( void );
};

#endif
