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
#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0

#include <ESP32Time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebSrv.h>
#include <time.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "AWSGPS.h"
#include "AstroWeatherStation.h"
#include "AWSWeb.h"
#include "AWSConfig.h"
#include "SQM.h"

#include "AWSSensorManager.h"

const char *_anemometer_model[3] = { "PR-3000-FSJT-N01", "GD-FS-RS485", "VMS-3003-CFSFX-N01" };
const uint64_t _anemometer_cmd[3] = { 0x010300000001840a, 0x010300000001840a, 0x010300000002c40b };
const uint16_t _anemometer_speed[3] = { 4800, 9600, 4800 };

const char *_windvane_model[3] = { "PR-3000-FXJT-N01", "GD-FX-RS485", "VMS-3003-CFSFX-N01" };
const uint64_t _windvane_cmd[3] = { 0x010300000002c40b, 0x020300000002c438, 0x010300000002c40b };
const uint16_t _windvane_speed[3] = { 4800, 9600, 4800 };

SemaphoreHandle_t sensors_read_mutex = NULL;

AWSSensorManager::AWSSensorManager( void )
{
	bme = new Adafruit_BME280();
	mlx = new Adafruit_MLX90614;
	tsl = new Adafruit_TSL2591( 2591 );
	rg9 = new HardwareSerial( RG9_UART );
	sqm = new SQM( tsl, &sensor_data );

	available_sensors = 0;
	rg9_initialised = 0;
	sensor_data = {0};

	i2c_mutex = xSemaphoreCreateMutex();
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

bool AWSSensorManager::initialise( I2C_SC16IS750 *sc16is750, AWSConfig *_config, bool _debug_mode )
{
	char	string[64];
	uint8_t mac[6];
	
	config = _config;
	debug_mode = _debug_mode;

	solar_panel = ( config->get_pwr_mode() == panel );
	snprintf( string, 64, "%s_%d", ESP.getChipModel(), ESP.getChipRevision() );
	sensor_data.ota_board = strdup( string );

	esp_read_mac( mac, ESP_MAC_WIFI_STA );
	snprintf( string, 64, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	sensor_data.ota_device = strdup( string );

	snprintf( string, 64, "%d-%s-%s-%s",config->get_pwr_mode(), config->get_pcb_version(), REV, BUILD_DATE );
	sensor_data.ota_config = strdup( string );

	// FIXME: needed?
	ESP32Time rtc( 0 );

	if ( !solar_panel ) {

		initialise_sensors( sc16is750 );
		sensors_read_mutex = xSemaphoreCreateMutex();
		std::function<void(void *)> _poll_sensors_task = std::bind( &AWSSensorManager::poll_sensors_task, this, std::placeholders::_1 );
		xTaskCreatePinnedToCore( 
			[](void *param) {
        	    std::function<void(void*)>* poll_proxy = static_cast<std::function<void(void*)>*>( param );
        	    (*poll_proxy)( NULL );
			}, "SensorsTask", 3000, &_poll_sensors_task, 10, &sensors_task_handle, 1 );
	}
	
	return true;
}

void AWSSensorManager::initialise_anemometer( void )
{
	anemometer.device = new SoftwareSerial( GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX );
	anemometer.device->begin( _anemometer_speed[ config->get_anemometer_model() ] );
	uint64_t_to_uint8_t_array( _anemometer_cmd[ config->get_anemometer_model() ], anemometer.request_cmd );
	pinMode( GPIO_ANEMOMETER_CTRL, OUTPUT );
}

void AWSSensorManager::initialise_BME( void )
{
	if ( !bme->begin( 0x76 ) )

		Serial.println( "[ERROR] Could not find BME280." );

	else {

		if ( debug_mode )
			Serial.println( "[INFO] Found BME280." );

		available_sensors |= BME_SENSOR;
	}
}

void AWSSensorManager::initialise_GPS( I2C_SC16IS750 *sc16is750 )
{
	if ( debug_mode )
		Serial.printf( "[DEBUG] Initialising GPS.\n" );

	gps = new AWSGPS( debug_mode );
	if ( gps->initialise( &sensor_data.gps, sc16is750, i2c_mutex )) {

		gps->start();
		gps->pilot_rtc( true );
		available_sensors |= GPS_SENSOR;
		delay( 1000 );						// Wait a little to get a fix

	} else
		Serial.printf( "[ERROR] GPS initialisation failed.\n" );

}

void AWSSensorManager::initialise_MLX( void )
{
	if ( !mlx->begin() )

		Serial.println( "[ERROR] Could not find MLX90614" );

	else {

		if ( debug_mode )
			Serial.println( "[INFO] Found MLX90614" );

		available_sensors |= MLX_SENSOR;
	}
}

bool AWSSensorManager::initialise_RG9( void )
{
	const int	bps[ RG9_SERIAL_SPEEDS ] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600 };
	char		str[ 128 ],
				status;
	uint8_t		l;
	
	for ( uint8_t i = 0; i < RG9_PROBE_RETRIES; i++ ) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Probing RG9, attempt #%d: ...", i );
			
		for ( uint8_t j = 0; j < RG9_SERIAL_SPEEDS; j++ ) {

			rg9->begin( bps[j], SERIAL_8N1, GPIO_RG9_RX, GPIO_RG9_TX );
			rg9->println();
			rg9->println();

			if ( debug_mode ) {

				Serial.printf( " @ %dbps: got [", bps[j] );
				Serial.flush();
			}

			rg9->println( "B" );
			l = RG9_read_string( str, 128 );

			if ( debug_mode )
				Serial.printf( "%s] ", str );

			if ( !strncmp( str, "Baud ", 5 )) {

				char bitrate[8];
				strncpy( bitrate, str+5, l-5 );
				Serial.printf( "%s[INFO] Found RG9 @ %s bps\n", debug_mode?"\n":"", bitrate );
				available_sensors |= RG9_SENSOR;
				status = RG9_OK;
				goto handle_status;
			}
			
			if ( !strncmp( str, "Reset " , 6 )) {

				char reset_cause = str[6];
				Serial.printf( "%s[INFO] Found RG9 @ %dbps after it was reset because of '%s'\n[INFO] RG9 boot message:\n", debug_mode?"\n":"", bps[j], RG9_reset_cause( reset_cause ));

				while (( l = RG9_read_string(  str, 128 )))
					Serial.println( str );

				available_sensors |= RG9_SENSOR;
				status = reset_cause;
				goto handle_status;
			}

			rg9->end();
		}
		if ( debug_mode )
			printf( "\n" );
	}

	Serial.printf( "[ERROR] Could not find RG9, resetting sensor.\n" );

	pinMode( GPIO_RG9_MCLR, OUTPUT );
	digitalWrite( GPIO_RG9_MCLR, LOW );
	delay( 500 );
	digitalWrite( GPIO_RG9_MCLR, HIGH );

	status = RG9_FAIL;

handle_status:

	// FIXME: restore alarms
	switch( status ) {
	
		case RG9_OK:
			rg9_initialised = true;
			break;
		case RG9_FAIL:
			rg9_initialised = false;
			break;
		case 'B':
			rg9_initialised = true;
//			send_alarm( runtime_config, "RG9 low voltage", "", debug_mode );
			break;
		case 'O':
			rg9_initialised = true;
	//		send_alarm( runtime_config, "RG9 problem", "Reset because of stack overflow, report problem to support.", debug_mode );
			break;
		case 'U':
			rg9_initialised = true;
		//	send_alarm( runtime_config, "RG9 problem", "Reset because of stack underflow, report problem to support.", debug_mode );
			break;
		default:
			Serial.printf( "[INFO] Unhandled RG9 reset code: %d. Report to support.\n", status );
			rg9_initialised = false;
			break;
	}
	return rg9_initialised;
}

void AWSSensorManager::initialise_sensors( I2C_SC16IS750 *_sc16is750 )
{
	if ( config->get_has_bme() )
		initialise_BME();
		
	if ( config->get_has_mlx() )
		initialise_MLX();
		
	if ( config->get_has_tsl() ) {
		
		initialise_TSL();
		sqm->set_msas_calibration_offset( config->get_msas_calibration_offset() );
		sqm->set_debug_mode( debug_mode );
	}
	
	if ( config->get_has_ws() )
		initialise_anemometer();
		
	// Model 0x02 (VMS-3003-CFSFX-N01) is a 2-in-1 device
	if ( config->get_has_wv() && ( config->get_wind_vane_model() != 0x02 ))	
		initialise_wind_vane();
		
	if ( config->get_has_rg9() )
		initialise_RG9();

	if ( config->get_has_gps() )
		initialise_GPS( _sc16is750 );
}

void AWSSensorManager::initialise_TSL( void )
{
	if ( !tsl->begin() ) {

		if ( debug_mode )
			Serial.println( "[ERROR] Could not find TSL2591" );

	} else {

		if ( debug_mode )
			Serial.println( "[INFO] Found TSL2591" );

		tsl->setGain( TSL2591_GAIN_LOW );
		tsl->setTiming( TSL2591_INTEGRATIONTIME_100MS );
		available_sensors |= TSL_SENSOR;
	}
}

void AWSSensorManager::initialise_wind_vane( void )
{
	wind_vane.device = new SoftwareSerial( GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX );
	wind_vane.device->begin( _windvane_speed[ config->get_wind_vane_model() ] );
	pinMode( GPIO_WIND_VANE_CTRL, OUTPUT );
	uint64_t_to_uint8_t_array( _windvane_cmd[ config->get_wind_vane_model() ], wind_vane.request_cmd );
}

void AWSSensorManager::read_anemometer( void )
{
	uint8_t		answer[7] = {0},
				i = 0,
				j;

	if ( debug_mode ) {
		
		Serial.printf( "[DEBUG] Sending command to the anemometer:" );
		for ( j = 0; j < 8; Serial.printf( " %02x", anemometer.request_cmd[ j++ ] ));
		Serial.printf( "\n" );
	}

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_ANEMOMETER_CTRL, SEND );
		anemometer.device->write( anemometer.request_cmd, 8 );
		anemometer.device->flush();

		digitalWrite( GPIO_ANEMOMETER_CTRL, RECV );
		anemometer.device->readBytes( answer, 7 );

		if ( debug_mode ) {

			Serial.print( "[DEBUG] Anemometer answer: " );
			for ( j = 0; j < 7; j++ )
				Serial.printf( "%02x ", answer[j] );

		}

		if ( answer[1] == 0x03 ) {

			if ( config->get_anemometer_model() != 0x02 ) 

				sensor_data.wind_speed = static_cast<float>(answer[4]) / 10.F;

			else {

				sensor_data.wind_speed = static_cast<float>( (answer[3]<< 8) + answer[4] ) / 100.F;
				sensor_data.wind_direction = ( answer[5] << 8 ) + answer[6];

				if ( debug_mode )
					Serial.printf( "\n[DEBUG] Wind direction: %d°", sensor_data.wind_direction );
			}
			
			if ( debug_mode )
				Serial.printf( "\n[DEBUG] Wind speed: %02.2f m/s\n", sensor_data.wind_speed );

		} else {

			if ( debug_mode )
				Serial.println( "(Error)." );
			delay( 500 );
		}

		if ( ++i == ANEMOMETER_MAX_TRIES ) {

			if ( config->get_anemometer_model() == 0x02 ) 
				available_sensors &= ~WIND_VANE_SENSOR;

			available_sensors &= ~ANEMOMETER_SENSOR;
			sensor_data.wind_speed = -1.F;
			return;
		}
	}

	if ( config->get_anemometer_model() == 0x02 ) 
		available_sensors |= WIND_VANE_SENSOR;

	available_sensors |= ANEMOMETER_SENSOR;
}

void AWSSensorManager::read_battery_level( void )
{
	int		adc_value = 0;
	float	adc_v_in,
			bat_v;
	
	if ( debug_mode )
		Serial.print( "[DEBUG] Battery level: " );

	digitalWrite( GPIO_BAT_ADC_EN, HIGH );
	delay( 500 );
	for( uint8_t i = 0; i < 5; i++ ) {

		adc_value += analogRead( GPIO_BAT_ADC );
		delay( 100 );
	}
	adc_value = static_cast<int>( adc_value / 5 );
	
	sensor_data.battery_level = ( adc_value >= ADC_V_MIN ) ? map( adc_value, ADC_V_MIN, ADC_V_MAX, 0, 100 ) : 0;

	if ( debug_mode ) {

		adc_v_in = static_cast<float>(adc_value) * VCC / ADC_V_MAX;
		bat_v = adc_v_in * ( V_DIV_R1 + V_DIV_R2 ) / V_DIV_R2;
		Serial.printf( "%03.2f%% (ADC value=%d, ADC voltage=%1.3fV, battery voltage=%1.3fV)\n", sensor_data.battery_level, adc_value, adc_v_in / 1000.F, bat_v / 1000.F );
	}

	digitalWrite( GPIO_BAT_ADC_EN, LOW );
}

void AWSSensorManager::read_BME( void  )
{
	float gammaM;
	
	if ( ( available_sensors & BME_SENSOR ) == BME_SENSOR ) {

		sensor_data.temperature = bme->readTemperature();
		sensor_data.pressure = bme->readPressure() / 100.F;
		sensor_data.rh = bme->readHumidity();

		// "Arden Buck" equation
		gammaM = log( ( sensor_data.rh / 100 )*exp( ( 18.68 - sensor_data.temperature / 234.5 ) * ( sensor_data.temperature / ( 257.14 + sensor_data.temperature )) ));
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
	sensor_data.rh = -1.F;
	sensor_data.dew_point = -99.F;
}

void AWSSensorManager::read_GPS( void )
{
	char buf[32];

	if ( sensor_data.gps.fix ) {
		
		if ( debug_mode ) {

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


void AWSSensorManager::read_RG9( void  )
{
	char	msg[128];

	if (( !rg9_initialised ) && ( !initialise_RG9() )) {

		Serial.printf( "[INFO] Not returning rain data.\n" );
		return;
	}
		
	rg9->println( "R" );
	memset( msg, 0, 128 );
	RG9_read_string( msg, 128 );

	if ( debug_mode )
		Serial.printf( "[DEBUG] RG9 STATUS = [%s]\n", msg );

	sensor_data.rain_intensity = static_cast<uint8_t>( msg[2] - '0' );
}

void AWSSensorManager::read_sensors( void )
{
	char	string[64];
	
	// TODO: handle gps time
//	if ( ntp_synced = sync_time() )
//		gettimeofday( &sensor_data.ntp_time, NULL );

	
	if ( solar_panel ) {

		pinMode( GPIO_BAT_ADC_EN, OUTPUT );
		delay( 100 );

		time( &sensor_data.timestamp );
		read_battery_level();

		pinMode( GPIO_ENABLE_3_3V, OUTPUT );
		pinMode( GPIO_ENABLE_12V, OUTPUT );
		digitalWrite( GPIO_ENABLE_3_3V, HIGH );
		digitalWrite( GPIO_ENABLE_12V, HIGH );

		delay( 500 );		// MLX96014 seems to take some time to properly initialise
		initialise_sensors( NULL );

		retrieve_sensor_data();

		digitalWrite( GPIO_ENABLE_3_3V, LOW );
		digitalWrite( GPIO_ENABLE_12V, LOW );

	} else
		sensor_data.battery_level = 100.0L;		// When running on 12V, convention is to report 100%

	if ( sensor_data.battery_level <= BAT_LEVEL_MIN ) {

		memset( string, 0, 64 );
		snprintf( string, 64, "LOW Battery level = %03.2f%%\n", sensor_data.battery_level );

		// Deal with ADC output accuracy, no need to stress about it, a few warnings are enough to get the message through :-)
/*		if (( low_battery_event_count++ >= LOW_BATTERY_COUNT_MIN ) && ( low_battery_event_count++ <= LOW_BATTERY_COUNT_MAX ))
			send_alarm( runtime_config, "Low battery", string, debug_mode );
*/
		if ( debug_mode )
			Serial.printf( "[DEBUG] %s", string );

	}
/*	
	if ( prev_available_sensors != available_sensors ) {

		prev_available_sensors = available_sensors;
		report_unavailable_sensors( runtime_config, available_sensors, debug_mode );
	}
*/
}

void AWSSensorManager::read_TSL( void )
{
	int			lux = -1;
	uint32_t	lum;
	uint16_t	ir,
				full;

	if ( ( available_sensors & TSL_SENSOR ) == TSL_SENSOR ) {

		lum = tsl->getFullLuminosity();
		ir = lum >> 16;
		full = lum & 0xFFFF;
		lux = tsl->calculateLux( full, ir );

		if ( debug_mode )
			Serial.printf( "[DEBUG] IR=%d FULL=%d VIS=%d Lux = %05d\n", ir, full, full - ir, lux );
	}
	
	// Avoid aberrant readings
	sensor_data.lux = ( lux < TSL_MAX_LUX ) ? lux : -1;
	sensor_data.irradiance = ( lux == -1 ) ? 0 : lux * LUX_TO_IRRADIANCE_FACTOR;
}

void AWSSensorManager::read_wind_vane( void )
{
	uint8_t		answer[7],
				i = 0,
				j;

	answer[1] = 0;

	if ( debug_mode ) {
		
		Serial.printf( "[DEBUG] Sending command to the wind vane:" );
		for ( j = 0; j < 8; Serial.printf( " %02x", wind_vane.request_cmd[ j++ ] ));
		Serial.printf( "\n" );
	}

	while ( answer[1] != 0x03 ) {

		if ( config->get_wind_vane_model() == 0x02 )
			digitalWrite( GPIO_ANEMOMETER_CTRL, SEND ); //FIXME: make it a struct item
		else
			digitalWrite( GPIO_WIND_VANE_CTRL, SEND );
		
		wind_vane.device->write( wind_vane.request_cmd, 8 );
		wind_vane.device->flush();

		if ( config->get_wind_vane_model() == 0x02 )
			digitalWrite( GPIO_ANEMOMETER_CTRL, RECV );
		else
			digitalWrite( GPIO_WIND_VANE_CTRL, RECV );

		wind_vane.device->readBytes( answer, 7 );

		if ( debug_mode ) {

			Serial.print( "[DEBUG] Wind vane answer : " );
			for ( j = 0; j < 6; j++ )
				Serial.printf( "%02x ", answer[j] );
		}

		if ( answer[1] == 0x03 ) {

			sensor_data.wind_direction = ( answer[5] << 8 ) + answer[6];

			if ( debug_mode )
				Serial.printf( "\n[DEBUG] Wind direction: %d°\n", sensor_data.wind_direction );

		} else {

			if ( debug_mode )
				Serial.println( "(Error)." );
			delay( 500 );
		}

		if ( ++i == WIND_VANE_MAX_TRIES ) {

			available_sensors &= ~WIND_VANE_SENSOR;
			sensor_data.wind_direction = -1;
			return;
		}
	}
	available_sensors |= WIND_VANE_SENSOR;

}

void AWSSensorManager::reset_rain_event( void )
{
	rain_event = false;
}

void AWSSensorManager::set_rain_event( void )
{
	rain_event = true;
}

void AWSSensorManager::retrieve_sensor_data( void )
{
	if ( xSemaphoreTake( i2c_mutex, 500 / portTICK_PERIOD_MS ) == pdTRUE ) {

		sensor_data.rain_event = rain_event;
		time( &sensor_data.timestamp );
		
		if ( config->get_has_bme() )
			read_BME();

		if ( config->get_has_mlx() )
			read_MLX();
		
		if ( config->get_has_tsl() ) {

			// FIXME: what if mlx is down?
			read_TSL();
			if ( ( available_sensors & TSL_SENSOR ) == TSL_SENSOR )
				sqm->read_SQM( sensor_data.ambient_temperature );		// TSL and MLX are in the same enclosure
		}
	
		if ( config->get_has_ws() )
			read_anemometer();

		// Wind direction has been read along with the wind speed as it is a 2-in-1 device
		if ( config->get_has_wv() && ( config->get_wind_vane_model() != 0x02 ))
			read_wind_vane();

		xSemaphoreGive( i2c_mutex );

		if ( config->get_has_rg9() )
			read_RG9();

		if ( config->get_has_gps() )
			read_GPS();

	}
}

uint8_t AWSSensorManager::RG9_read_string( char *str, uint8_t len )
{
	uint8_t i;
	
	delay( 500 );

	if ( rg9->available() > 0 ) {

		i = rg9->readBytes( str, len );
		if ( i >= 2 )
			str[ i-2 ] = 0;	// trim trailing \n
		return i-1;
	}
	return 0;
}

const char *AWSSensorManager::RG9_reset_cause( char code )
{
	switch ( code ) {
		case 'N': return "Normal power up";
		case 'M': return "MCLR";
		case 'W': return "Watchdog timer reset";
		case 'O': return "Start overflow";
		case 'U': return "Start underflow";
		case 'B': return "Low voltage";
		case 'D': return "Other";
		default: return "Unknown";
	}
}

void AWSSensorManager::poll_sensors_task( void *dummy )
{
	while( true ) {

		if ( xSemaphoreTake( sensors_read_mutex, 5000 / portTICK_PERIOD_MS ) == pdTRUE ) {

			retrieve_sensor_data();
			xSemaphoreGive( sensors_read_mutex );

		}
		delay( 15000 );
	}
}

void AWSSensorManager::uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array )
{
	uint8_t i = 0;
    for ( i = 0; i < 8; i++ )
		cmd_array[ i ] = (uint8_t)( ( cmd >> (56-(8*i))) & 0xff );
}
