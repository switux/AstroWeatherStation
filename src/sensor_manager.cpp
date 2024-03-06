/*
  	AWSSensorManager.cpp

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

#include <esp_task_wdt.h>
#include <ESP32Time.h>
// Keep these two to get rid of compile time errors because of incompatibilities between libraries
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "AWSGPS.h"
#include "common.h"
#include "config_manager.h"
#include "SQM.h"
#include "device.h"
#include "Hydreon.h"
#include "anemometer.h"
#include "wind_vane.h"
#include "sensor_manager.h"
#include "AstroWeatherStation.h"

RTC_DATA_ATTR long	prev_available_sensors = 0;
RTC_DATA_ATTR long	available_sensors = 0;

SemaphoreHandle_t sensors_read_mutex = NULL;	// Issue #7
const aws_device_t ALL_SENSORS	= ( aws_device_t::MLX_SENSOR | aws_device_t::TSL_SENSOR | aws_device_t::BME_SENSOR | aws_device_t::WIND_VANE_SENSOR | aws_device_t::ANEMOMETER_SENSOR | aws_device_t::RAIN_SENSOR | aws_device_t::GPS_SENSOR );

extern AstroWeatherStation station;

aws_device_t operator&( aws_device_t a, aws_device_t b )
{
	return static_cast<aws_device_t>( static_cast<unsigned long>(a) & static_cast<unsigned long>(b) );
}

bool operator!=( unsigned long a, aws_device_t b )
{
	return a != static_cast<unsigned long>(b);
}

aws_device_t operator&( unsigned long a, aws_device_t b )
{
	return static_cast<aws_device_t>( a & static_cast<unsigned long>(b) );
}

aws_device_t operator~( aws_device_t b )
{
	return static_cast<aws_device_t>( ~static_cast<unsigned long>(b) );
}

aws_device_t operator|( aws_device_t a, aws_device_t b )
{
	return static_cast<aws_device_t>( static_cast<unsigned long>(a) | static_cast<unsigned long>(b) );
}

aws_device_t operator*( aws_device_t a, int b )
{
	return static_cast<aws_device_t>( static_cast<unsigned long>(a) * b );
}

aws_device_t& operator|=( aws_device_t& a, aws_device_t b )
{
	return a = static_cast<aws_device_t>( static_cast<unsigned long>(a) | static_cast<unsigned long>(b) );
}

aws_device_t& operator&=( aws_device_t& a, aws_device_t b )
{
	return a = static_cast<aws_device_t>( static_cast<unsigned long>(a) & static_cast<unsigned long>(b) );
}

AWSSensorManager::AWSSensorManager( void ) :
	bme( new Adafruit_BME280() ),
	mlx( new Adafruit_MLX90614() ),
	tsl( new Adafruit_TSL2591( 2591 )),
	i2c_mutex( xSemaphoreCreateMutex() )
{
	memset( &sensor_data, 0, sizeof( sensor_data_t ));
}

aws_device_t AWSSensorManager::get_available_sensors( void )
{
	return available_sensors;
}

bool AWSSensorManager::get_debug_mode( void )
{
	return debug_mode;
}

SemaphoreHandle_t AWSSensorManager::get_i2c_mutex( void )
{
	return i2c_mutex;
}

sensor_data_t *AWSSensorManager::get_sensor_data( void )
{
	return &sensor_data;
}

bool AWSSensorManager::initialise( I2C_SC16IS750 *sc16is750, AWSConfig *_config, bool _rain_event )
{
	config = _config;
	rain_event = _rain_event;
	initialise_sensors( sc16is750 );

	if ( !solar_panel ) {

		sensors_read_mutex = xSemaphoreCreateMutex();
		std::function<void(void *)> _poll_sensors_task = std::bind( &AWSSensorManager::poll_sensors_task, this, std::placeholders::_1 );
		xTaskCreatePinnedToCore(
			[](void *param) {	// NOSONAR
        	    std::function<void(void*)>* poll_proxy = static_cast<std::function<void(void*)>*>( param );
        	    (*poll_proxy)( NULL );
			}, "SensorManagerTask", 10000, &_poll_sensors_task, 5, &sensors_task_handle, 1 );
	}
	k[0] = config->get_parameter<int>( "k1" );
	k[1] = config->get_parameter<int>( "k2" );
	k[2] = config->get_parameter<int>( "k3" );
	k[3] = config->get_parameter<int>( "k4" );
	k[4] = config->get_parameter<int>( "k5" );
	k[5] = config->get_parameter<int>( "k6" );
	k[6] = config->get_parameter<int>( "k7" );
	
	return true;
}

void AWSSensorManager::initialise_BME( void )
{
	if ( !bme->begin( 0x76 ) )

		Serial.printf( "[ERROR] Could not find BME280.\n" );

	else {

		if ( debug_mode )
			Serial.printf( "[INFO] Found BME280.\n" );

		available_sensors |= aws_device_t::BME_SENSOR;
	}
}

void AWSSensorManager::initialise_MLX( void )
{
	if ( !mlx->begin() )

		Serial.printf( "[ERROR] Could not find MLX90614.\n" );

	else {

		if ( debug_mode )
			Serial.printf( "[INFO] Found MLX90614.\n" );

		available_sensors |= aws_device_t::MLX_SENSOR;
	}
}

void AWSSensorManager::initialise_sensors( I2C_SC16IS750 *_sc16is750 )
{
	if ( solar_panel ) {

		pinMode( GPIO_ENABLE_3_3V, OUTPUT );
		pinMode( GPIO_ENABLE_12V, OUTPUT );
		digitalWrite( GPIO_ENABLE_3_3V, HIGH );
		digitalWrite( GPIO_ENABLE_12V, HIGH );

		delay( 500 );		// MLX96014 seems to take some time to properly initialise
	}

	if ( !rain_event && config->get_has_device( aws_device_t::BME_SENSOR ) )
		initialise_BME();

	if ( !rain_event && config->get_has_device( aws_device_t::MLX_SENSOR ) )
		initialise_MLX();

	if ( !rain_event && config->get_has_device( aws_device_t::TSL_SENSOR ) ) {

		initialise_TSL();
		sqm.initialise( tsl, &sensor_data.sqm, config->get_parameter<float>( "msas_calibration_offset" ), debug_mode );
	}

	if ( !rain_event &&  config->get_has_device( aws_device_t::ANEMOMETER_SENSOR ) ) {

		if ( !anemometer.initialise( &rs485_bus, polling_ms_interval, config->get_parameter<int>( "anemometer_model" ), debug_mode ))

			available_sensors &= ~aws_device_t::ANEMOMETER_SENSOR;

		else {

			Serial.printf( "[INFO] Found anemometer.\n" );
			available_sensors |= aws_device_t::ANEMOMETER_SENSOR;
		}
	}

	if ( !rain_event && config->get_has_device( aws_device_t::WIND_VANE_SENSOR ) ) {

		if ( !wind_vane.initialise( &rs485_bus, config->get_parameter<int>( "wind_vane_model" ), debug_mode ))

			available_sensors &= ~aws_device_t::WIND_VANE_SENSOR;

		else {

			Serial.printf( "[INFO] Found wind vane.\n" );
			available_sensors |= aws_device_t::WIND_VANE_SENSOR;
		}
	}

	if ( config->get_has_device( aws_device_t::RAIN_SENSOR ) && initialise_rain_sensor())
		available_sensors |= aws_device_t::RAIN_SENSOR;

}

bool AWSSensorManager::initialise_rain_sensor( void )
{
	return rain_sensor.initialise( Serial1, debug_mode );
}

void AWSSensorManager::initialise_TSL( void )
{
	if ( !tsl->begin() ) {

		if ( debug_mode )
			Serial.printf( "[ERROR] Could not find TSL2591.\n" );

	} else {

		if ( debug_mode )
			Serial.printf( "[INFO] Found TSL2591.\n" );

		tsl->setGain( TSL2591_GAIN_LOW );
		tsl->setTiming( TSL2591_INTEGRATIONTIME_100MS );
		available_sensors |= aws_device_t::TSL_SENSOR;
	}
}

bool AWSSensorManager::poll_sensors( void )
{
	if ( xSemaphoreTake( sensors_read_mutex, 2000 / portTICK_PERIOD_MS ) == pdTRUE ) {

		retrieve_sensor_data();
		xSemaphoreGive( sensors_read_mutex );
		sensor_data.weather.wind_gust = anemometer.get_wind_gust();
		return true;
	}
	return false;
}

void AWSSensorManager::poll_sensors_task( void *dummy )	// NOSONAR
{
	while( true ) {

		if ( sensor_data.weather.rain_event )
			station.send_alarm( "[Station] RAIN EVENT", "Rain event!" );
			
		if ( xSemaphoreTake( sensors_read_mutex, 5000 / portTICK_PERIOD_MS ) == pdTRUE ) {

			retrieve_sensor_data();
			xSemaphoreGive( sensors_read_mutex );
			if ( config->get_has_device( aws_device_t::ANEMOMETER_SENSOR ) )
				sensor_data.weather.wind_gust = anemometer.get_wind_gust();
			else
				sensor_data.weather.wind_gust = 0.F;
			sensor_data.available_sensors = available_sensors;
		}
		delay( polling_ms_interval );
	}
}

const char *AWSSensorManager::rain_intensity_str( void )
{
	return rain_sensor.get_rain_intensity_str();
}

void AWSSensorManager::read_anemometer( void )
{
	float	x;

	if ( !anemometer.get_initialised() )
		return;

	if ( ( x = anemometer.get_wind_speed( true ) ) == -1 )
		return;

	sensor_data.weather.wind_speed = x;
}

void AWSSensorManager::read_BME( void  )
{
	if ( ( available_sensors & aws_device_t::BME_SENSOR ) == aws_device_t::BME_SENSOR ) {

		sensor_data.weather.temperature = bme->readTemperature();
		sensor_data.weather.pressure = bme->readPressure() / 100.F;
		sensor_data.weather.rh = bme->readHumidity();

		// "Arden Buck" equation
		float gammaM = log( ( sensor_data.weather.rh / 100 )*exp( ( 18.68 - sensor_data.weather.temperature / 234.5 ) * ( sensor_data.weather.temperature / ( 257.14 + sensor_data.weather.temperature )) ));
		if ( sensor_data.weather.temperature >= 0 )
			sensor_data.weather.dew_point = ( 238.88 * gammaM ) / ( 17.368 - gammaM );
		else
			sensor_data.weather.dew_point = ( 247.15 * gammaM ) / ( 17.966 - gammaM );

		if ( debug_mode ) {

			Serial.printf( "[DEBUG] Temperature = %2.2f °C\n", sensor_data.weather.temperature  );
			Serial.printf( "[DEBUG] Pressure = %4.2f hPa\n", sensor_data.weather.pressure );
			Serial.printf( "[DEBUG] RH = %3.2f %%\n", sensor_data.weather.rh );
			Serial.printf( "[DEBUG] Dew point = %2.2f °C\n", sensor_data.weather.dew_point );
		}
		return;
	}

	sensor_data.weather.temperature = -99.F;
	sensor_data.weather.pressure = 0.F;
	sensor_data.weather.rh = 0.F;
	sensor_data.weather.dew_point = -99.F;
}

void AWSSensorManager::read_MLX( void )
{
	if ( ( available_sensors & aws_device_t::MLX_SENSOR ) == aws_device_t::MLX_SENSOR ) {

		sensor_data.weather.ambient_temperature = mlx->readAmbientTempC();
		sensor_data.weather.sky_temperature = mlx->readObjectTempC();
		sensor_data.weather.raw_sky_temperature = mlx->readObjectTempC();

		if ( config->get_parameter<int>( "cloud_coverage_formula" ) == 0 ) {
			sensor_data.weather.sky_temperature -= sensor_data.weather.ambient_temperature;
			sensor_data.weather.cloud_coverage = ( sensor_data.weather.sky_temperature <= -15 ) ? 0 : 2;
		}
		else {

			float t = ( k[0] / 100 ) * ( sensor_data.weather.ambient_temperature - k[1] / 10 ) * ( k[2] / 100 ) * pow( exp( k[3] / 1000 * sensor_data.weather.ambient_temperature ), k[4]/100 );
			float t67;
			if ( abs( k[1] / 10 - sensor_data.weather.ambient_temperature ) < 1 )
				t67 = sign<int>( k[5] ) * sign<float>( sensor_data.weather.ambient_temperature - k[1]/10) * ( k[1] / 10 - sensor_data.weather.ambient_temperature );
			else
				t67 = k[5] / 10 * sign( sensor_data.weather.ambient_temperature - k[1]/10 ) * ( log( abs( k[1]/10 - sensor_data.weather.ambient_temperature ))/log(10) + k[6] / 100 );
			t += t67;
			sensor_data.weather.sky_temperature -= t;

		}
		if ( debug_mode )
			Serial.printf( "[DEBUG] Ambient temperature = %2.2f °C / Raw sky temperature = %2.2f °C / Corrected sky temperature = %2.2f\n", sensor_data.weather.ambient_temperature, sensor_data.weather.raw_sky_temperature, sensor_data.weather.sky_temperature );
		return;
	}
	sensor_data.weather.ambient_temperature = -99.F;
	sensor_data.weather.raw_sky_temperature = -99.F;
	sensor_data.weather.sky_temperature = -99.F;
}

void AWSSensorManager::read_rain_sensor( void )
{
	sensor_data.weather.rain_intensity = rain_sensor.get_rain_intensity();
}

void AWSSensorManager::read_sensors( void )
{
	retrieve_sensor_data();

	digitalWrite( GPIO_ENABLE_3_3V, LOW );
	digitalWrite( GPIO_ENABLE_12V, LOW );
	
	if ( prev_available_sensors != available_sensors ) {

		prev_available_sensors = static_cast<unsigned long>( available_sensors );
		station.report_unavailable_sensors();
	}
}

void AWSSensorManager::read_TSL( void )
{
	int			lux = -1;

	if ( ( available_sensors & aws_device_t::TSL_SENSOR ) == aws_device_t::TSL_SENSOR ) {

		uint32_t lum = tsl->getFullLuminosity();
		uint16_t ir = lum >> 16;
		uint16_t full = lum & 0xFFFF;
		lux = tsl->calculateLux( full, ir );

		if ( debug_mode )
			Serial.printf( "[DEBUG] Infrared=%05d Full=%05d Visible=%05d Lux = %05d\n", ir, full, full - ir, lux );
	}

	// Avoid aberrant readings
	sensor_data.sun.lux = ( lux < TSL_MAX_LUX ) ? lux : -1;
	sensor_data.sun.irradiance = ( lux == -1 ) ? 0 : lux * LUX_TO_IRRADIANCE_FACTOR;
}

void AWSSensorManager::read_wind_vane( void )
{
	int16_t	x;

	if ( !wind_vane.get_initialised() )
		return;

	if ( ( x = wind_vane.get_wind_direction( true ) ) == -1 )
		return;

	sensor_data.weather.wind_direction = x;
}

void AWSSensorManager::reset_rain_event( void )
{
	rain_event = false;
}

void AWSSensorManager::retrieve_sensor_data( void )
{
	if ( xSemaphoreTake( i2c_mutex, 500 / portTICK_PERIOD_MS ) == pdTRUE ) {

		sensor_data.weather.rain_event = rain_event;
		sensor_data.timestamp = station.get_timestamp();

		if ( config->get_has_device( aws_device_t:: BME_SENSOR ) )
			read_BME();

		esp_task_wdt_reset();

		if ( config->get_has_device( aws_device_t::MLX_SENSOR ) )
			read_MLX();

		esp_task_wdt_reset();

		if ( config->get_has_device( aws_device_t::TSL_SENSOR ) ) {

			read_TSL();
			if ( ( available_sensors & aws_device_t::TSL_SENSOR ) == aws_device_t::TSL_SENSOR )
				sqm.read( sensor_data.weather.ambient_temperature );
		}

		if ( config->get_has_device( aws_device_t::ANEMOMETER_SENSOR ) )
			read_anemometer();

		esp_task_wdt_reset();

		if ( config->get_has_device( aws_device_t::WIND_VANE_SENSOR ) )
			read_wind_vane();

		esp_task_wdt_reset();

		xSemaphoreGive( i2c_mutex );

		if ( config->get_has_device( aws_device_t::RAIN_SENSOR ) ) {
			sensor_data.weather.rain_intensity = rain_sensor.get_rain_intensity();
			if ( !sensor_data.weather.rain_intensity )
				sensor_data.weather.rain_event = false;
		}
	}
}

void AWSSensorManager::set_debug_mode( bool b )
{
	debug_mode = b;
}

void AWSSensorManager::set_rain_event( void )
{
	rain_event = true;
}

void AWSSensorManager::set_solar_panel( bool b )
{
	solar_panel = b;
	
	if ( solar_panel ) {

		pinMode( GPIO_BAT_ADC_EN, OUTPUT );
		pinMode( GPIO_BAT_ADC, INPUT );

	}
}

void AWSSensorManager::resume( void )
{
	vTaskResume( sensors_task_handle );
}

bool AWSSensorManager::sensor_is_available( aws_device_t sensor )
{
	return (( available_sensors & sensor ) == sensor );
}

void AWSSensorManager::suspend( void )
{
	vTaskSuspend( sensors_task_handle );
}
