/*
  	AWSSensorManager.cpp

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
#include "sensor.h"
#include "Hydreon.h"
#include "anemometer.h"
#include "wind_vane.h"
#include "sensor_manager.h"
#include "AstroWeatherStation.h"

RTC_DATA_ATTR uint16_t low_battery_event_count = 0;
//RTC_DATA_ATTR byte	prev_available_sensors = 0;
//RTC_DATA_ATTR byte	available_sensors = 0;

SemaphoreHandle_t sensors_read_mutex = NULL;

extern AstroWeatherStation station;

AWSSensorManager::AWSSensorManager( void ) :
	bme( new Adafruit_BME280() ),
	mlx( new Adafruit_MLX90614() ),
	tsl( new Adafruit_TSL2591( 2591 )),
	sqm( new SQM( tsl, &sensor_data )),
	gps( nullptr ),
	config( nullptr ),
	available_sensors( 0 ),
	rain_event( false ),
	i2c_mutex( xSemaphoreCreateMutex() ),
	polling_ms_interval( DEFAULT_SENSOR_POLLING_MS_INTERVAL )
{
	hw_version[ 0 ] = 0;
	memset( &sensor_data, 0, sizeof( sensor_data_t ));
}

uint8_t AWSSensorManager::get_available_sensors( void )
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

	// FIXME: needed?
	ESP32Time rtc( 0 );

	rain_event = _rain_event;
	initialise_sensors( sc16is750 );

	if ( !solar_panel ) {

		sensors_read_mutex = xSemaphoreCreateMutex();
		std::function<void(void *)> _poll_sensors_task = std::bind( &AWSSensorManager::poll_sensors_task, this, std::placeholders::_1 );
		xTaskCreatePinnedToCore(
			[](void *param) {
        	    std::function<void(void*)>* poll_proxy = static_cast<std::function<void(void*)>*>( param );
        	    (*poll_proxy)( NULL );
			}, "SensorsTask", 3000, &_poll_sensors_task, 5, &sensors_task_handle, 1 );
	}

	return true;
}

void AWSSensorManager::initialise_BME( void )
{
	if ( !bme->begin( 0x76 ) )

		Serial.printf( "[ERROR] Could not find BME280.\n" );

	else {

		if ( debug_mode )
			Serial.printf( "[INFO] Found BME280.\n" );

		available_sensors |= BME_SENSOR;
	}
}

void AWSSensorManager::initialise_GPS( I2C_SC16IS750 *sc16is750 )
{
	if ( debug_mode )
		Serial.printf( "[DEBUG] Initialising GPS.\n" );

	gps = new AWSGPS( debug_mode );

	if ( ( config->get_has_sc16is750() && !gps->initialise( &sensor_data.gps, sc16is750, i2c_mutex )) || !gps->initialise( &sensor_data.gps )) {

			Serial.printf( "[ERROR] GPS initialisation failed.\n" );
			return;
	}
	gps->start();
	gps->pilot_rtc( true );
	available_sensors |= GPS_SENSOR;
	delay( 1000 );						// Wait a little to get a fix

}

void AWSSensorManager::initialise_MLX( void )
{
	if ( !mlx->begin() )

		Serial.printf( "[ERROR] Could not find MLX90614.\n" );

	else {

		if ( debug_mode )
			Serial.printf( "[INFO] Found MLX90614.\n" );

		available_sensors |= MLX_SENSOR;
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

	if ( !rain_event && config->get_has_bme() )
		initialise_BME();

	if ( !rain_event && config->get_has_mlx() )
		initialise_MLX();

	if ( !rain_event && config->get_has_tsl() ) {

		initialise_TSL();
		sqm->set_msas_calibration_offset( config->get_msas_calibration_offset() );
		sqm->set_debug_mode( debug_mode );
	}

	if ( !rain_event &&  config->get_has_ws() ) {

		if ( !anemometer.initialise( &rs485_bus, polling_ms_interval, config->get_anemometer_model(), debug_mode ))

			available_sensors &= ~ANEMOMETER_SENSOR;

		else {

			Serial.printf( "[INFO] Found anemometer.\n" );
			available_sensors |= ANEMOMETER_SENSOR;
		}
	}

	if ( !rain_event && config->get_has_wv() ) {

		if ( !wind_vane.initialise( &rs485_bus, config->get_wind_vane_model(), debug_mode ))

			available_sensors &= ~WIND_VANE_SENSOR;

		else {

			Serial.printf( "[INFO] Found wind vane.\n" );
			available_sensors |= WIND_VANE_SENSOR;
		}
	}

	if ( config->get_has_rain_sensor() && initialise_rain_sensor())
		available_sensors |= RAIN_SENSOR;

	if ( !rain_event && config->get_has_gps() )
		initialise_GPS( _sc16is750 );
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
		available_sensors |= TSL_SENSOR;
	}
}

bool AWSSensorManager::poll_sensors( void )
{
	if ( xSemaphoreTake( sensors_read_mutex, 2000 / portTICK_PERIOD_MS ) == pdTRUE ) {

		retrieve_sensor_data();
		xSemaphoreGive( sensors_read_mutex );
		sensor_data.wind_gust = anemometer.get_wind_gust();
		return true;
	}
	return false;
}

void AWSSensorManager::poll_sensors_task( void *dummy )
{
	while( true ) {

		if ( xSemaphoreTake( sensors_read_mutex, 5000 / portTICK_PERIOD_MS ) == pdTRUE ) {

			retrieve_sensor_data();
			xSemaphoreGive( sensors_read_mutex );
			if ( config->get_has_ws() )
				sensor_data.wind_gust = anemometer.get_wind_gust();
			else
				sensor_data.wind_gust = 0.F;

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

	sensor_data.wind_speed = x;
}

void AWSSensorManager::read_battery_level( void )
{
	int		adc_value = 0;

	WiFi.mode ( WIFI_OFF );

	if ( debug_mode )
		Serial.print( "[DEBUG] Battery level: " );

	digitalWrite( GPIO_BAT_ADC_EN, HIGH );
	delay( 500 );

	for( uint8_t i = 0; i < 5; i++ ) {

		adc_value += analogRead( GPIO_BAT_ADC );
	}
	adc_value = static_cast<int>( adc_value / 5 );
	sensor_data.battery_level = ( adc_value >= ADC_V_MIN ) ? map( adc_value, ADC_V_MIN, ADC_V_MAX, 0, 100 ) : 0;

	if ( debug_mode ) {

		float adc_v_in = adc_value * VCC / ADC_V_MAX;
		float bat_v = adc_v_in * ( V_DIV_R1 + V_DIV_R2 ) / V_DIV_R2;
		Serial.printf( "%03.2f%% (ADC value=%d, ADC voltage=%1.3fV, battery voltage=%1.3fV)\n", sensor_data.battery_level, adc_value, adc_v_in / 1000.F, bat_v / 1000.F );
	}

	digitalWrite( GPIO_BAT_ADC_EN, LOW );
}

void AWSSensorManager::read_BME( void  )
{
	if ( ( available_sensors & BME_SENSOR ) == BME_SENSOR ) {

		sensor_data.temperature = bme->readTemperature();
		sensor_data.pressure = bme->readPressure() / 100.F;
		sensor_data.rh = bme->readHumidity();

		// "Arden Buck" equation
		float gammaM = log( ( sensor_data.rh / 100 )*exp( ( 18.68 - sensor_data.temperature / 234.5 ) * ( sensor_data.temperature / ( 257.14 + sensor_data.temperature )) ));
		if ( sensor_data.temperature >= 0 )
			sensor_data.dew_point = ( 238.88 * gammaM ) / ( 17.368 - gammaM );
		else
			sensor_data.dew_point = ( 247.15 * gammaM ) / ( 17.966 - gammaM );

		if ( debug_mode ) {

			Serial.printf( "[DEBUG] Temperature = %2.2f °C\n", sensor_data.temperature  );
			Serial.printf( "[DEBUG] Pressure = %4.2f hPa\n", sensor_data.pressure );
			Serial.printf( "[DEBUG] RH = %3.2f %%\n", sensor_data.rh );
			Serial.printf( "[DEBUG] Dew point = %2.2f °C\n", sensor_data.dew_point );
		}
		return;
	}

	sensor_data.temperature = -99.F;
	sensor_data.pressure = 0.F;
	sensor_data.rh = 0.F;
	sensor_data.dew_point = -99.F;
}

void AWSSensorManager::read_GPS( void )
{

	if ( sensor_data.gps.fix ) {

		if ( debug_mode ) {

			// flawfinder: ignore
			char buf[32];
			strftime( buf, 32, "%Y-%m-%d %H:%M:%S", localtime( &sensor_data.gps.time.tv_sec ) );
			Serial.printf( "[DEBUG] GPS FIX. LAT=%f LON=%f ALT=%f DATETIME=%s\n", sensor_data.gps.latitude, sensor_data.gps.longitude, sensor_data.gps.altitude, buf );
		}

	} else

		if ( debug_mode )
			Serial.printf( "[DEBUG] NO GPS FIX.\n" );

}

void AWSSensorManager::read_MLX( void )
{
	if ( ( available_sensors & MLX_SENSOR ) == MLX_SENSOR ) {

		sensor_data.ambient_temperature = mlx->readAmbientTempC();
		sensor_data.sky_temperature = mlx->readObjectTempC();

		if ( debug_mode ) {

			Serial.print( "[DEBUG] Ambient temperature = " );
			Serial.print( sensor_data.ambient_temperature );
			Serial.print( "°C / Raw sky temperature = " );
			Serial.print( sensor_data.sky_temperature );
			Serial.printf( "°C\n" );
		}
		return;
	}
	sensor_data.ambient_temperature = -99.F;
	sensor_data.sky_temperature = -99.F;
}

void AWSSensorManager::read_rain_sensor( void )
{
	sensor_data.rain_intensity = rain_sensor.get_rain_intensity();
}

void AWSSensorManager::read_sensors( void )
{

	if ( solar_panel ) {

		retrieve_sensor_data();

		digitalWrite( GPIO_ENABLE_3_3V, LOW );
		digitalWrite( GPIO_ENABLE_12V, LOW );

		if ( sensor_data.battery_level <= BAT_LEVEL_MIN ) {

			// flawfinder: ignore
			char string[64] = {0};
			snprintf( string, 64, "LOW Battery level = %03.2f%%\n", sensor_data.battery_level );

			// Deal with ADC output accuracy, no need to stress about it, a few warnings are enough to get the message through :-)
			if (( low_battery_event_count++ >= LOW_BATTERY_COUNT_MIN ) && ( low_battery_event_count++ <= LOW_BATTERY_COUNT_MAX ))
				station.send_alarm( "Low battery", string );

			if ( debug_mode )
				Serial.printf( "[DEBUG] %s", string );
		}

	} else
		sensor_data.battery_level = 100.0L;		// When running on 12V, convention is to report 100%

/*	if ( prev_available_sensors != available_sensors ) {

		prev_available_sensors = available_sensors;
		station.report_unavailable_sensors( available_sensors );
	}
*/
}

void AWSSensorManager::read_TSL( void )
{
	int			lux = -1;

	if ( ( available_sensors & TSL_SENSOR ) == TSL_SENSOR ) {

		uint32_t lum = tsl->getFullLuminosity();
		uint16_t ir = lum >> 16;
		uint16_t full = lum & 0xFFFF;
		lux = tsl->calculateLux( full, ir );

		if ( debug_mode )
			Serial.printf( "[DEBUG] Infrared=%05d Full=%05d Visible=%05d Lux = %05d\n", ir, full, full - ir, lux );
	}

	// Avoid aberrant readings
	sensor_data.lux = ( lux < TSL_MAX_LUX ) ? lux : -1;
	sensor_data.irradiance = ( lux == -1 ) ? 0 : lux * LUX_TO_IRRADIANCE_FACTOR;
}

void AWSSensorManager::read_wind_vane( void )
{
	int16_t	x;

	if ( !wind_vane.get_initialised() )
		return;

	if ( ( x = wind_vane.get_wind_direction( true ) ) == -1 )
		return;

	sensor_data.wind_direction = x;
}

void AWSSensorManager::reset_rain_event( void )
{
	rain_event = false;
}

void AWSSensorManager::retrieve_sensor_data( void )
{
	if ( xSemaphoreTake( i2c_mutex, 500 / portTICK_PERIOD_MS ) == pdTRUE ) {

		sensor_data.rain_event = rain_event;
		if ( station.is_ntp_synced() )
			time( &sensor_data.timestamp );

		if ( config->get_has_bme() )
			read_BME();

		esp_task_wdt_reset();

		if ( config->get_has_mlx() )
			read_MLX();

		esp_task_wdt_reset();

		if ( config->get_has_tsl() ) {

			// FIXME: what if mlx is down, since we get the temperature from it?
			read_TSL();
			if ( ( available_sensors & TSL_SENSOR ) == TSL_SENSOR )
				sqm->read_SQM( sensor_data.ambient_temperature );		// TSL and MLX are in the same enclosure
		}

		if ( config->get_has_ws() )
			read_anemometer();

		esp_task_wdt_reset();

		if ( config->get_has_wv() )
			read_wind_vane();

		esp_task_wdt_reset();

		xSemaphoreGive( i2c_mutex );

		if ( config->get_has_rain_sensor() )
			sensor_data.rain_intensity = rain_sensor.get_rain_intensity();

		if ( config->get_has_gps() )
			read_GPS();

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
