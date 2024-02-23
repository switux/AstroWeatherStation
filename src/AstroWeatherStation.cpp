/*  	
	AstroWeatherStation.cpp

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

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <Preferences.h>
#include <time.h>
#include <thread>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.h>
#include <ESPAsyncWebSrv.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPping.h>
#include <stdio.h>
#include <stdarg.h>
#include <FS.h>
#include <SPIFFS.h>
#include <charconv>
#include <optional>
#include <TinyGPSPlus.h>

#include "Embedded_Template_Library.h"
#include "etl/string.h"
#include <ESP32OTAPull.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "common.h"
#include "AWSGPS.h"
#include "sensor_manager.h"
#include "config_manager.h"
#include "config_server.h"
#include "dome.h"
#include "AWSLookout.h"
#include "alpaca_server.h"
#include "AstroWeatherStation.h"


extern void IRAM_ATTR		_handle_rain_event( void );
extern SemaphoreHandle_t	sensors_read_mutex;		// FIXME: hide this within the sensor manager

std::array<std::string, 3> PWR_MODE_STR = { "Solar panel", "12V DC", "PoE" };

const bool				FORMAT_SPIFFS_IF_FAILED = true;
const unsigned long		CONFIG_MODE_GUARD		= 5000000;	// 5 seconds

RTC_DATA_ATTR time_t 	rain_event_timestamp = 0;		// NOSONAR
RTC_DATA_ATTR time_t 	boot_timestamp = 0;				// NOSONAR
RTC_DATA_ATTR time_t 	last_ntp_time = 0;				// NOSONAR
RTC_DATA_ATTR uint16_t	ntp_time_misses = 0;			// NOSONAR
RTC_DATA_ATTR bool		catch_rain_event = false;		// NOSONAR
RTC_DATA_ATTR uint16_t 	low_battery_event_count = 0;	// NOSONAR

AstroWeatherStation::AstroWeatherStation( void )
{
	station_data.health.init_heap_size = xPortGetFreeHeapSize();
	station_data.health.current_heap_size = station_data.health.init_heap_size;
	station_data.health.largest_free_heap_block = heap_caps_get_largest_free_block( MALLOC_CAP_8BIT );
	location = DEFAULT_LOCATION;
}

void AstroWeatherStation::check_ota_updates( void )
{
	ESP32OTAPull	ota;
	etl::string<64>	string;
	int				ret_code;
	uint8_t			*wifi_mac = network.get_wifi_mac();

	snprintf( string.data(), string.capacity(), "%s_%d", ESP.getChipModel(), ESP.getChipRevision() );
	ota.OverrideBoard( string.data() );
	snprintf( string.data(), string.capacity(), "%02x:%02x:%02x:%02x:%02x:%02x", wifi_mac[0], wifi_mac[1], wifi_mac[2], wifi_mac[3], wifi_mac[4], wifi_mac[5] );
	ota.OverrideDevice( string.data() );
	ota.SetConfig( unique_build_id.data() );
	ota.SetCallback( OTA_callback );
	Serial.printf( "[INFO] Checking for OTA firmware update.\n" );
	ret_code = ota.CheckForOTAUpdate( "https://www.datamancers.net/images/AWS.json", REV.data() );
	Serial.printf( "[INFO] Firmware OTA update result: %s.\n", OTA_message( ret_code ));
}

void AstroWeatherStation::check_rain_event_guard_time( uint16_t guard_time )
{
	time_t now = get_timestamp();

	if ( catch_rain_event )
		return;
		
	if ( ( now - rain_event_timestamp ) <= guard_time )
		return;

	if ( debug_mode )
		Serial.printf( "\n[DEBUG] Rain event guard time elapsed.\n" );

	pinMode( GPIO_RAIN_SENSOR_RAIN, INPUT );
	if ( !solar_panel )
		attachInterrupt( GPIO_RAIN_SENSOR_RAIN, _handle_rain_event, FALLING );
	catch_rain_event = true;
}

void AstroWeatherStation::compute_uptime( void )
{
	if ( !on_solar_panel() )

		station_data.health.uptime = round( esp_timer_get_time() / 1000000 );

 	else {

		time_t now;
		time( &now );
		if ( !boot_timestamp ) {

			station_data.health.uptime = round( esp_timer_get_time() / 1000000 );
			boot_timestamp = now - station_data.health.uptime;

		} else

			station_data.health.uptime = now - boot_timestamp;
	}
}

bool AstroWeatherStation::determine_boot_mode( void )
{
 	unsigned long start					= micros();
	unsigned long button_pressed_secs	= 0;

	pinMode( GPIO_DEBUG, INPUT );

	debug_mode = static_cast<bool>( 1 - gpio_get_level( GPIO_DEBUG )) || DEBUG_MODE;
	while ( !( gpio_get_level( GPIO_DEBUG ))) {

		if (( micros() - start ) <= CONFIG_MODE_GUARD )
			break;
		delay( 100 );
		button_pressed_secs = micros() - start;
	}

	return ( button_pressed_secs > CONFIG_MODE_GUARD );
}

void AstroWeatherStation::display_banner()
{
	if ( !debug_mode )
		return;

	uint8_t	*wifi_mac = network.get_wifi_mac();
	int		i;
	
	Serial.printf( "\n#############################################################################################\n" );
	Serial.printf( "# AstroWeatherStation                                                                       #\n" );
	Serial.printf( "#  (c) Lesage Franck - lesage@datamancers.net                                               #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# HARDWARE SETUP                                                                            #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# MCU               : Model %s Revision %d", ESP.getChipModel(), ESP.getChipRevision() );
	print_config_string( "# WIFI Mac          : %02x:%02x:%02x:%02x:%02x:%02x", wifi_mac[0], wifi_mac[1], wifi_mac[2], wifi_mac[3], wifi_mac[4], wifi_mac[5] );
	print_config_string( "# Power Mode        : %s", PWR_MODE_STR[ static_cast<int>( config.get_pwr_mode()) ].data() );
	print_config_string( "# PCB version       : %s", config.get_pcb_version().data() );
	print_config_string( "# Ethernet present  : %s", config.get_has_device( ETHERNET_DEVICE ) ? "Yes" : "No" );
	print_config_string( "# GPIO ext. present : %s", config.get_has_device( SC16IS750_DEVICE ) ? "Yes" : "No" );
	print_config_string( "# Firmware          : %s-%s", REV.data(), BUILD_DATE );

	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# GPIO PIN CONFIGURATION                                                                    #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# Wind sensors : RX=%d TX=%d CTRL=%d", GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX, GPIO_WIND_SENSOR_CTRL );
	print_config_string( "# Rain sensor  : RX=%d TX=%d MCLR=%d RAIN=%d", GPIO_RAIN_SENSOR_RX, GPIO_RAIN_SENSOR_TX, GPIO_RAIN_SENSOR_MCLR, GPIO_RAIN_SENSOR_RAIN );
	if ( solar_panel ) {

		print_config_string( "# 3.3V SWITCH  : %d", GPIO_ENABLE_3_3V );
		print_config_string( "# 12V SWITCH   : %d", GPIO_ENABLE_12V );
		print_config_string( "# BAT LVL      : SW=%d ADC=%d", GPIO_BAT_ADC_EN, GPIO_BAT_ADC );
	}
	print_config_string( "# DEBUG        : %d", GPIO_DEBUG );

	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# RUNTIME CONFIGURATION                                                                     #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_runtime_config();

	Serial.printf( "#############################################################################################\n" );
}

void AstroWeatherStation::enter_config_mode( void )
{
	if ( debug_mode )
		Serial.printf( "[INFO] Entering config mode...\n ");

	start_config_server();
	Serial.printf( "[ERROR] Failed to start WiFi AP, cannot enter config mode.\n ");
}

etl::string_view AstroWeatherStation::get_anemometer_sensorname( void )
{
	if ( config.get_has_device( ANEMOMETER_SENSOR ) )
		return etl::string_view( config.get_anemometer_model_str() );
	return etl::string_view( "N/A" );
}

uint16_t AstroWeatherStation::get_config_port( void )
{
	return config.get_parameter<int>( "config_port" );
}

bool AstroWeatherStation::get_debug_mode( void )
{
	return debug_mode;
}

Dome *AstroWeatherStation::get_dome( void )
{
	return &station_devices.dome;
}

byte AstroWeatherStation::get_eth_cidr_prefix( void )
{
	return network.get_eth_cidr_prefix();
}

bool AstroWeatherStation::get_location_coordinates( float *latitude, float *longitude )
{
	if ( station_data.gps.fix ) {

		*longitude = station_data.gps.longitude;
		*latitude = station_data.gps.latitude;
		return true;
	}
	return false;
}

IPAddress *AstroWeatherStation::get_eth_dns( void )
{
	return network.get_eth_dns();
}

IPAddress *AstroWeatherStation::get_eth_gw( void )
{
	return network.get_eth_gw();
}

IPAddress *AstroWeatherStation::get_eth_ip( void )
{
	return network.get_eth_ip();
}

etl::string_view AstroWeatherStation::get_json_sensor_data( void )
{
	DynamicJsonDocument	json_data(768);
	sensor_data_t		*sensor_data = sensor_manager.get_sensor_data();

	json_data["battery_level"] = station_data.health.battery_level;
	json_data["timestamp"] = sensor_data->timestamp;
	json_data["rain_event"] = sensor_data->weather.rain_event;
	json_data["temperature"] = sensor_data->weather.temperature;
	json_data["pressure"] = sensor_data->weather.pressure;
	json_data["sl_pressure"] = sensor_data->weather.sl_pressure;
	json_data["rh"] = sensor_data->weather.rh;
	json_data["ota_board"] = ota_setup.board;
	json_data["ota_device"] = ota_setup.device;
	json_data["ota_config"] = ota_setup.config;
	json_data["wind_speed"] = sensor_data->weather.wind_speed;
	json_data["wind_gust"] = sensor_data->weather.wind_gust;
	json_data["wind_direction"] = sensor_data->weather.wind_direction;
	json_data["dew_point"] = sensor_data->weather.dew_point;
	json_data["rain_intensity"] = sensor_data->weather.rain_intensity;
	json_data["sky_temperature"] = sensor_data->weather.sky_temperature;
	json_data["ambient_temperature"] = sensor_data->weather.ambient_temperature;
	json_data["cloud_coverage"] = sensor_data->weather.cloud_coverage;
	json_data["msas"] = sensor_data->sqm.msas;
	json_data["nelm"] = sensor_data->sqm.nelm;
	json_data["integration_time"] = sensor_data->sqm.integration_time;
	json_data["gain"] = sensor_data->sqm.gain;
	json_data["ir_luminosity"] = sensor_data->sqm.ir_luminosity;
	json_data["full_luminosity"] = sensor_data->sqm.full_luminosity;
	json_data["lux"] = sensor_data->sun.lux;
	json_data["irradiance"] = sensor_data->sun.irradiance;
	json_data["ntp_time_sec"] = station_data.ntp_time.tv_sec;
	json_data["ntp_time_usec"] = station_data.ntp_time.tv_usec;
	json_data["gps_fix"] = station_data.gps.fix;
	json_data["gps_longitude"] = station_data.gps.longitude;
	json_data["gps_latitude"] = station_data.gps.latitude;
	json_data["gps_altitude"] = station_data.gps.altitude;
	json_data["gps_time_sec"] = station_data.gps.time.tv_sec;
	json_data["gps_time_usec"] = station_data.gps.time.tv_usec;
	json_data["uptime"] = get_uptime();
	json_data["init_heap_size"] = station_data.health.init_heap_size;
	json_data["current_heap_size"] = station_data.health.current_heap_size;
	json_data["largest_free_heap_block" ] = station_data.health.largest_free_heap_block;

	if ( measureJson( json_data ) > json_sensor_data.capacity() ) {

		Serial.printf( "[BUG] sensor_data json is too small. Please report to support!\n" );
		return etl::string_view( "" );

	}
	int l = serializeJson( json_data, json_sensor_data.data(), json_sensor_data.capacity() );
	if ( debug_mode )
		Serial.printf( "[DEBUG] sensor_data is %d bytes long, max size is %d bytes.\n", l, json_sensor_data.capacity() );
	return json_sensor_data;
}

etl::string_view AstroWeatherStation::get_json_string_config( void )
{
	return etl::string_view( config.get_json_string_config() );
}

etl::string_view AstroWeatherStation::get_location( void )
{
	return etl::string_view( location );
}

etl::string_view AstroWeatherStation::get_root_ca( void )
{
	return config.get_root_ca();
}

sensor_data_t *AstroWeatherStation::get_sensor_data( void )
{
	return sensor_manager.get_sensor_data();
}

station_data_t *AstroWeatherStation::get_station_data( void )
{
	return &station_data;
}

etl::string_view AstroWeatherStation::get_unique_build_id( void )
{
	return unique_build_id;
}

time_t AstroWeatherStation::get_timestamp( void )
{
	time_t now;

	if ( ntp_synced )
		time( &now );
	else
		now = last_ntp_time + ( US_SLEEP / 1000000 ) * ntp_time_misses;

	return now;
}

uint32_t AstroWeatherStation::get_uptime( void )
{
	compute_uptime();
	return station_data.health.uptime;
}

byte AstroWeatherStation::get_wifi_sta_cidr_prefix( void )
{
	return network.get_wifi_sta_cidr_prefix();
}

IPAddress *AstroWeatherStation::get_wifi_sta_dns( void )
{
	return network.get_wifi_sta_dns();
}

IPAddress *AstroWeatherStation::get_wifi_sta_gw( void )
{
	return network.get_wifi_sta_gw();
}

IPAddress *AstroWeatherStation::get_wifi_sta_ip( void )
{
	return network.get_wifi_sta_ip();
}

etl::string_view AstroWeatherStation::get_wind_vane_sensorname( void )
{
	if ( config.get_has_device( WIND_VANE_SENSOR ))
		return etl::string_view( config.get_wind_vane_model_str() );
	return etl::string_view( "N/A" );
}

void AstroWeatherStation::handle_dome_shutter_is_closed( void )
{
	station_devices.dome.shutter_is_closed();
}

void AstroWeatherStation::handle_dome_shutter_is_moving( void )
{
	station_devices.dome.shutter_is_moving();
}

void AstroWeatherStation::handle_dome_shutter_is_opening( void )
{
	station_devices.dome.shutter_is_opening();
}

void AstroWeatherStation::handle_rain_event( void )
{
	if ( solar_panel ) {

		sensor_manager.read_rain_sensor();
		send_rain_event_alarm( sensor_manager.rain_intensity_str() );

	} else

		detachInterrupt( GPIO_RAIN_SENSOR_RAIN );

	time( &rain_event_timestamp );
	sensor_manager.set_rain_event();
	lookout.set_rain_event();
}

bool AstroWeatherStation::has_gps( void )
{
	return config.get_has_device( GPS_SENSOR );
}

bool AstroWeatherStation::has_rain_sensor( void )
{
	return config.get_has_device( RAIN_SENSOR );
}

bool AstroWeatherStation::initialise( void )
{
	etl::string<64>			string;
	std::array<uint8_t,6>	mac;

	bool					config_mode = determine_boot_mode();

	Serial.printf( "\n\n[INFO] AstroWeatherStation [REV %s, BUILD %s] is booting...\n", REV.data(), BUILD_DATE );

	if ( !config.load( debug_mode ) )
		return false;

	solar_panel = ( static_cast<aws_pwr_src>( config.get_pwr_mode()) == aws_pwr_src::panel );
	sensor_manager.set_solar_panel( solar_panel );
	sensor_manager.set_debug_mode( debug_mode );

	// FIXME: all this OTA setup is messy, use etl::string and drop strdup()
	snprintf( string.data(), string.capacity(), "%s_%d", ESP.getChipModel(), ESP.getChipRevision() );
	ota_setup.board = strdup( string.data() );

	if ( ( esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED ) && solar_panel )
		boot_timestamp = 0;

	snprintf( unique_build_id.data(), string.capacity(), "%d-%s-%s-%s",config.get_pwr_mode(), config.get_pcb_version().data(), REV.data(), BUILD_DATE );

	esp_read_mac( mac.data(), ESP_MAC_WIFI_STA );
	snprintf( string.data(), string.capacity(), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	ota_setup.device = strdup( string.data() );

	snprintf( string.data(), string.capacity(), "%d-%s-%s-%s", config.get_pwr_mode(), config.get_pcb_version().data(), REV, BUILD_DATE );
	ota_setup.config = strdup( unique_build_id.data() );

	if ( solar_panel )
		read_battery_level();

	// The idea is that if we did something stupid with the config and the previous setup was correct, we can restore it, otherwise, move on ...
	if ( !network.initialise( &config, debug_mode ) && config.can_rollback() )
		return config.rollback();

	if ( solar_panel ) {

		//FIXME: we already get this to cater for station uptime
		esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
		rain_event = ( ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason );

		if ( config_mode )
			enter_config_mode();

	} else

		start_config_server();

	if ( !startup_sanity_check() && config.can_rollback() )
		return config.rollback();

	display_banner();

	// Do not enable earlier as some HW configs rely on SC16IS750 to pilot the dome.
	if ( config.get_has_device( DOME_DEVICE ) ) {

	    if ( config.get_has_device( SC16IS750_DEVICE ) ) {

			//FIXME: make guard time a config parameter
			station_devices.dome.initialise( &station_devices.sc16is750, sensor_manager.get_i2c_mutex(), 30, debug_mode );

	    } else {

			station_devices.dome.initialise( 30, debug_mode );
	    }
	}

	if ( solar_panel ) {

		sync_time( true );
		if ( config.get_has_device( RAIN_SENSOR ))
			check_rain_event_guard_time( config.get_parameter<int>( "rain_event_guard_time" ) );
	}

	if ( rain_event ) {

		sensor_manager.initialise( &station_devices.sc16is750, &config, rain_event );
		handle_rain_event();
		return true;
	}

	if ( config.get_has_device( GPS_SENSOR ) )
		initialise_GPS();

	if ( !sensor_manager.initialise( &station_devices.sc16is750, &config, false ))
		return false;

	if ( solar_panel )
		return true;
		
	start_alpaca_server();
	
	if ( config.get_has_device( RAIN_SENSOR ) ) {

		Serial.printf( "[INFO] Monitoring rain sensor for rain event.\n" );
		pinMode( GPIO_RAIN_SENSOR_RAIN, INPUT );
		attachInterrupt( GPIO_RAIN_SENSOR_RAIN, _handle_rain_event, FALLING );
	}

	if ( config.get_parameter<bool>( "lookout_enabled" ))
		lookout.initialise( &config, &sensor_manager, &station_devices.dome, debug_mode );
	else
		station_devices.dome.close_shutter();	// FIXME: add parameter to set default behaviour

	std::function<void(void *)> _feed = std::bind( &AstroWeatherStation::periodic_tasks, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
			std::function<void(void*)>* periodic_tasks_proxy = static_cast<std::function<void(void*)>*>( param );	// NOSONAR
			(*periodic_tasks_proxy)( NULL );
		}, "GPSFeed", 2000, &_feed, 5, &aws_periodic_task_handle, 1 );

	return true;
}

void AstroWeatherStation::initialise_GPS( void )
{
	if ( debug_mode )
		Serial.printf( "[DEBUG] Initialising GPS.\n" );

	if ( ( config.get_has_device( SC16IS750_DEVICE ) && !station_devices.gps.initialise( &station_data.gps, &station_devices.sc16is750, sensor_manager.get_i2c_mutex() )) || !station_devices.gps.initialise( &station_data.gps )) {

			Serial.printf( "[ERROR] GPS initialisation failed.\n" );
			return;
	}
	station_devices.gps.start();
	station_devices.gps.pilot_rtc( true );
	delay( 1000 );						// Wait a little to get a fix

}

void AstroWeatherStation::initialise_sensors( void )
{
	sensor_manager.initialise_sensors( &station_devices.sc16is750 );
}

bool AstroWeatherStation::is_ntp_synced( void )
{
	return ntp_synced;
}

bool AstroWeatherStation::is_rain_event( void )
{
	return rain_event;
}

bool AstroWeatherStation::is_sensor_initialised( uint8_t sensor_id )
{
	return ( sensor_manager.get_available_sensors() & sensor_id );
}

bool AstroWeatherStation::on_solar_panel( void )
{
	return solar_panel;
}

void OTA_callback( int offset, int total_length )
{
	static float	percentage = 0.F;
	float			p = ( 100.F * offset / total_length );

	if ( p - percentage > 10.F ) {

		Serial.printf("[INFO] Updating %d of %d (%02d%%)...\n", offset, total_length, 100 * offset / total_length );
		percentage = p;
	}
}

const char *AstroWeatherStation::OTA_message( int code )
{
	switch ( code ) {

		case ESP32OTAPull::UPDATE_AVAILABLE:
			return "An update is available but wasn't installed";

		case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
			return "No profile matches";

		case ESP32OTAPull::NO_UPDATE_AVAILABLE:
			return "Profile matched, but update not applicable";

		case ESP32OTAPull::UPDATE_OK:
			return "An update was done, but no reboot";

		case ESP32OTAPull::HTTP_FAILED:
			return "HTTP GET failure";

		case ESP32OTAPull::WRITE_ERROR:
			return "Write error";

		case ESP32OTAPull::JSON_PROBLEM:
			return "Invalid JSON";

		case ESP32OTAPull::OTA_UPDATE_FAIL:
			return "Update failure (no OTA partition?)";

		default:
			if ( code > 0 )
				return "Unexpected HTTP response code";
			break;
	}
	return "Unknown error";
}

void AstroWeatherStation::periodic_tasks( void *dummy )	// NOSONAR
{
	uint8_t		sync_time_mod = 1;
	uint16_t	rain_event_guard_time = config.get_parameter<int>( "rain_event_guard_time" );
	
	while ( true ) {

		if ( sync_time_mod % 5 )

			sync_time_mod++;

		else {

			station_data.health.current_heap_size =	xPortGetFreeHeapSize();
			station_data.health.largest_free_heap_block = heap_caps_get_largest_free_block( MALLOC_CAP_8BIT );
			sync_time( false );
			sync_time_mod = 1;
		}
		time_t now;

		if ( ntp_synced )
			time( &now );
		else
			now = last_ntp_time + ( US_SLEEP / 1000000 ) * ntp_time_misses;

		if ( config.get_has_device( RAIN_SENSOR ))
			check_rain_event_guard_time( rain_event_guard_time );

		delay( 1000 );
	}
}

bool AstroWeatherStation::poll_sensors( void )
{
	if ( config.get_has_device( GPS_SENSOR ) )
		read_GPS();
	return sensor_manager.poll_sensors();
}

template<typename... Args>
etl::string<96> AstroWeatherStation::format_helper( const char *fmt, Args... args )
{
	char buf[96];	// NOSONAR
	snprintf( buf, 95, fmt, args... );
	return etl::string<96>( buf );
}

template void AstroWeatherStation::print_config_string<>( const char *fmt );

template<typename... Args>
void AstroWeatherStation::print_config_string( const char *fmt, Args... args )
{
	etl::string<96>	string; 
 	byte 			i;
 	int				l;

	string = format_helper( fmt, args... );
	l = string.size();
	
	if ( l >= 0 ) {

		string.append( string.capacity() - l - 4, ' ' );
		string.append( "#\n" );
	}
	Serial.printf( "%s", string.data() );
}

void AstroWeatherStation::read_battery_level( void )
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
	adc_value = adc_value / 5;
	station_data.health.battery_level = ( adc_value >= ADC_V_MIN ) ? map( adc_value, ADC_V_MIN, ADC_V_MAX, 0, 100 ) : 0;

	if ( debug_mode ) {

		float adc_v_in = adc_value * VCC / ADC_V_MAX;
		float bat_v = adc_v_in * ( V_DIV_R1 + V_DIV_R2 ) / V_DIV_R2;
		Serial.printf( "%03.2f%% (ADC value=%d, ADC voltage=%1.3fV, battery voltage=%1.3fV)\n", station_data.health.battery_level, adc_value, adc_v_in / 1000.F, bat_v / 1000.F );
	}

	digitalWrite( GPIO_BAT_ADC_EN, LOW );
}

void AstroWeatherStation::read_GPS( void )
{

	if ( station_data.gps.fix ) {

		if ( debug_mode ) {

			// flawfinder: ignore
			etl::string<32> buf;
			struct tm dummy;
			strftime( buf.data(), buf.capacity(), "%Y-%m-%d %H:%M:%S", localtime_r( &station_data.gps.time.tv_sec, &dummy ) );
			Serial.printf( "[DEBUG] GPS FIX. LAT=%f LON=%f ALT=%f DATETIME=%s\n", station_data.gps.latitude, station_data.gps.longitude, station_data.gps.altitude, buf.data() );
		}

	} else

		if ( debug_mode )
			Serial.printf( "[DEBUG] NO GPS FIX.\n" );

}

int AstroWeatherStation::reformat_ca_root_line( std::array<char,97> &string, int str_len, int ca_pos, int ca_len, const char *root_ca )
{
	int string_pos;
	
	strlcat( string.data(), root_ca + ca_pos, 92 );
	for( string_pos = str_len; string_pos < 92; string_pos++ ) {

		if ( string[ string_pos ] == '\n' ) {

			if (( ca_pos < 92 ) || (( ca_len - ca_pos) < 92))

				string[ string_pos ] = ' ';

			else {

				memcpy( string.data() + string_pos, root_ca + ca_pos + 1, 96 - string_pos - 3 );
				ca_pos++;

			}
		}
		ca_pos++;
		if ( ca_pos > ca_len )
			break;
	}
	ca_pos--;
	for( int j = string_pos; j < 92; string[ j ] = ' ', j++ );
	memset( string.data() + 91, 0, 6 );
	strlcat( string.data(), " #\n", string.size() - 1 );
	return ca_pos;
}

void AstroWeatherStation::print_runtime_config( void )
{
 	std::array<char,97>	string;
	const char			*root_ca = config.get_root_ca().data();
	int					ca_pos = 0;

	print_config_string( "# AP SSID      : %s", config.get_parameter<const char *>( "wifi_ap_ssid" ));
	print_config_string( "# AP PASSWORD  : %s", config.get_parameter<const char *>( "wifi_ap_password" ));
	print_config_string( "# AP IP        : %s", config.get_parameter<const char *>( "wifi_ap_ip" ));
	print_config_string( "# AP Gateway   : %s", config.get_parameter<const char *>( "wifi_ap_gw" ));
	print_config_string( "# STA SSID     : %s", config.get_parameter<const char *>( "wifi_sta_ssid" ));
	print_config_string( "# STA PASSWORD : %s", config.get_parameter<const char *>( "wifi_sta_password" ));
	print_config_string( "# STA IP       : %s", config.get_parameter<const char *>( "wifi_sta_ip" ));
	print_config_string( "# STA Gateway  : %s", config.get_parameter<const char *>( "wifi_sta_gw" ));
	print_config_string( "# SERVER       : %s", config.get_parameter<const char *>( "remote_server" ));
	print_config_string( "# URL PATH     : /%s", config.get_parameter<const char *>( "url_path" ));
	print_config_string( "# TZNAME       : %s", config.get_parameter<const char *>( "tzname" ));

	memset( string.data(), 0, string.size() );
	int str_len = snprintf( string.data(), string.size() - 1, "# ROOT CA      : " );

	int ca_len = config.get_root_ca().size();

	while ( ca_pos < ca_len ) {

		ca_pos = reformat_ca_root_line( string, str_len, ca_pos, ca_len, root_ca );
		Serial.printf( "%s", string.data() );
		memset( string.data(), 0, string.size() );
		str_len = snprintf( string.data(), string.size() - 1, "# " );
	}

	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# SENSORS & CONTROLS                                                                        #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# DOME             : %s", config.get_has_device( DOME_DEVICE ) ? "Yes" : "No" );
	print_config_string( "# GPS              : %s", config.get_has_device( GPS_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# SQM/IRRANDIANCE  : %s", config.get_has_device( TSL_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# CLOUD SENSOR     : %s", config.get_has_device( MLX_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# RH/TEMP/PRES.    : %s", config.get_has_device( BME_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# WINDVANE         : %s", config.get_has_device( WIND_VANE_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# WINDVANE MODEL   : %s", config.get_wind_vane_model_str().data() );
	print_config_string( "# ANEMOMETER       : %s", config.get_has_device( ANEMOMETER_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# ANEMOMETER MODEL : %s", config.get_anemometer_model_str().data() );
	print_config_string( "# RAIN SENSOR      : %s", config.get_has_device( RAIN_SENSOR ) ? "Yes" : "No" );

}

bool AstroWeatherStation::rain_sensor_available( void )
{
	return sensor_manager.rain_sensor_available();
}

void AstroWeatherStation::read_sensors( void )
{
	sensor_manager.read_sensors();

	//FIXME: done during sensor_manager.retrieve_sensor_data()
	
	if ( station_data.health.battery_level <= BAT_LEVEL_MIN ) {

		etl::string<64> string;
		snprintf( string.data(), string.capacity(), "LOW Battery level = %03.2f%%\n", station_data.health.battery_level );

		// Deal with ADC output accuracy, no need to stress about it, a few warnings are enough to get the message through :-)
		if (( low_battery_event_count >= LOW_BATTERY_COUNT_MIN ) && ( low_battery_event_count <= LOW_BATTERY_COUNT_MAX ))
			send_alarm( "Low battery", string.data() );

		low_battery_event_count++;
		if ( debug_mode )
			Serial.printf( "[DEBUG] %s", string.data() );
	}
}

void AstroWeatherStation::reboot( void )
{
	ESP.restart();
}

void AstroWeatherStation::report_unavailable_sensors( void )
{
	std::array<std::string, 7> sensor_name = { "MLX96014 ", "TSL2591 ", "BME280 ", "WIND VANE ", "ANEMOMETER ", "RAIN_SENSOR ", "GPS " };
	std::array<char,96> unavailable_sensors;
	uint8_t		j = sensor_manager.get_available_sensors();
	uint8_t		k;

	strlcpy( unavailable_sensors.data(), "Unavailable sensors: ", unavailable_sensors.size() );

	k = j;
	for ( uint8_t i = 0; i < 7; i++ ) {

		if ( !( k & 1 ))
			strlcat( unavailable_sensors.data(), sensor_name[i].c_str(), 13 );
		k >>= 1;
	}

	if ( debug_mode ) {

		Serial.printf( "[DEBUG] %s", unavailable_sensors.data() );

		if ( j == ALL_SENSORS )
			Serial.printf( "none.\n" );
	}

	if ( j != ALL_SENSORS )
		send_alarm( "Unavailable sensors report", unavailable_sensors.data() );
}

void AstroWeatherStation::send_alarm( const char *subject, const char *message )
{
	DynamicJsonDocument content( 512 );
	// flawfinder: ignore
	char jsonString[ 600 ];	// NOSONAR
	content["subject"] = subject;
	content["message"] = message;

	serializeJson( content, jsonString );
	network.post_content( "alarm.php", strlen( "alarm.php" ), jsonString );
}

void AstroWeatherStation::send_backlog_data( void )
{
	etl::string<1024> line;
	bool	empty = true;

	SPIFFS.begin( FORMAT_SPIFFS_IF_FAILED );

	// flawfinder: ignore
	File backlog = SPIFFS.open( "/unsent.txt", FILE_READ );

	if ( !backlog ) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] No backlog data to send.\n" );
		return;
	}

	// flawfinder: ignore
	File new_backlog = SPIFFS.open( "/unsent.new", FILE_WRITE );

	while ( backlog.available() ) {

		int	i = backlog.readBytesUntil( '\n', line.data(), line.size() - 1 );
		if ( !i )
			break;
		line[i] = '\0';
		if ( !network.post_content( "newData.php", strlen( "newData.php" ), line.data() )) {

			empty = false;
			// flawfinder: ignore
			if ( new_backlog.printf( "%s\n", line.data() ) != ( 1 + line.size() ))
				Serial.printf( "[ERROR] Could not write data into the backlog.\n" );

		}
	}

	new_backlog.close();
	backlog.close();

	if ( !empty ) {

		SPIFFS.rename( "/unsent.new", "/unsent.txt" );
		if ( debug_mode )
			Serial.printf( "[DEBUG] Data backlog is not empty.\n" );

	} else {

		SPIFFS.remove( "/unsent.txt" );
		SPIFFS.remove( "/unsent.new" );
		if ( debug_mode )
			Serial.printf( "[DEBUG] Data backlog is empty.\n");
	}
}

void AstroWeatherStation::send_data( void )
{
	if ( !solar_panel ) {

		while ( xSemaphoreTake( sensors_read_mutex, 5000 /  portTICK_PERIOD_MS ) != pdTRUE )

			if ( debug_mode )
				Serial.printf( "[DEBUG] Waiting for sensor data update to complete.\n" );

	}

	get_json_sensor_data();

	if ( debug_mode )
    	Serial.printf( "[DEBUG] Sensor data: %s\n", json_sensor_data.data() );

	if ( network.post_content( "newData.php", strlen( "newData.php" ), json_sensor_data.data() ))
		send_backlog_data();
	else
		store_unsent_data( json_sensor_data );

	if ( !solar_panel )
		xSemaphoreGive( sensors_read_mutex );
}

void AstroWeatherStation::send_rain_event_alarm( const char *str )
{
	etl::string<32>	msg;

	snprintf( msg.data(), msg.capacity(), "RAIN! Level = %s", str );
	send_alarm( "Rain event", msg.data() );
}

void AstroWeatherStation::start_alpaca_server( void )
{
	switch ( static_cast<aws_iface>( config.get_parameter<int>( "alpaca_iface" ))) {

		case aws_iface::wifi_sta:
			alpaca.start( WiFi.localIP(), debug_mode );
			break;

		case aws_iface::wifi_ap:
			alpaca.start( WiFi.softAPIP(), debug_mode );
			break;

		case aws_iface::eth:
			alpaca.start( Ethernet.localIP(), debug_mode );
			break;

	}
}

bool AstroWeatherStation::start_config_server( void )
{
	server.initialise( debug_mode );
	return true;
}

bool AstroWeatherStation::startup_sanity_check( void )
{
	switch ( static_cast<aws_iface>( config.get_parameter<int>( "pref_iface" ) )) {

		case aws_iface::wifi_sta:
			return Ping.ping( WiFi.gatewayIP(), 3 );

		case aws_iface::eth:
			return Ping.ping( ETH.gatewayIP(), 3 );

		case aws_iface::wifi_ap:
			return true;
	}
	return false;
}

bool AstroWeatherStation::store_unsent_data( etl::string_view data )
{
	bool ok = false;

	SPIFFS.begin( FORMAT_SPIFFS_IF_FAILED );

	// flawfinder: ignore
	File backlog = SPIFFS.open( "/unsent.txt", FILE_APPEND );

	if ( !backlog ) {

		Serial.printf( "[ERROR] Cannot store data until server is available.\n" );
		return false;
	}

	// flawfinder: ignore
	if (( ok = ( backlog.printf( "%s\n", data.data() ) == ( data.size() + 1 )) )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Ok, data secured for when server is available. data=%s\n", data.data() );

	} else

		Serial.printf( "[ERROR] Could not write data.\n" );

	backlog.close();
	return ok;
}

bool AstroWeatherStation::sync_time( bool verbose )
{
	const char	*ntp_server = "pool.ntp.org";
	uint8_t		ntp_retries = 5;
   	struct 		tm timeinfo;

	if ( config.get_has_device( GPS_SENSOR ) && station_data.gps.fix )
		return true;

	if ( debug_mode && verbose )
		Serial.printf( "[DEBUG] Connecting to NTP server " );

	configTzTime( config.get_parameter<const char *>( "tzname" ), ntp_server );

	while ( !( ntp_synced = getLocalTime( &timeinfo )) && ( --ntp_retries > 0 ) ) {	// NOSONAR

		if ( debug_mode && verbose )
			Serial.printf( "." );
		delay( 1000 );
		configTzTime( config.get_parameter<const char *>( "tzname" ), ntp_server );
	}
	if ( debug_mode && verbose ) {

		Serial.printf( "\n[DEBUG] %sNTP Synchronised. ", ntp_synced ? "" : "NOT " );
		Serial.print( "Time and date: " );
		Serial.print( &timeinfo, "%Y-%m-%d %H:%M:%S\n" );
		Serial.printf( "\n" );
	}

	if ( ntp_synced ) {

		time( &sensor_manager.get_sensor_data()->timestamp );
		station_data.ntp_time.tv_sec = sensor_manager.get_sensor_data()->timestamp;
		if ( ntp_time_misses )
			ntp_time_misses = 0;
		last_ntp_time = sensor_manager.get_sensor_data()->timestamp;

	} else {

		// Not proud of this but it should be sufficient if the number of times we miss ntp sync is not too big
		ntp_time_misses++;
		sensor_manager.get_sensor_data()->timestamp =  last_ntp_time + ( US_SLEEP / 1000000 ) * ntp_time_misses;

	}
	return ntp_synced;
}

bool AstroWeatherStation::update_config( JsonVariant &proposed_config )
{
	return config.save_runtime_configuration( proposed_config );
}

void AstroWeatherStation::wakeup_reason_to_string( esp_sleep_wakeup_cause_t wakeup_reason, char *wakeup_string )
{
	switch ( wakeup_reason ) {

		case ESP_SLEEP_WAKEUP_EXT0 :
			snprintf( wakeup_string, 49, "Awakened by RTC IO: Rain event!" );
			break;

		case ESP_SLEEP_WAKEUP_TIMER :
			snprintf( wakeup_string, 49, "Awakened by timer" );
			break;

		default :
			snprintf( wakeup_string, 49, "Awakened by other: %d", wakeup_reason );
			break;
	}
}

AWSNetwork::AWSNetwork( void )
{
	ssl_eth_client = nullptr;
	current_wifi_mode = aws_wifi_mode::sta;
	current_pref_iface = aws_iface::wifi_sta;
	memset( wifi_mac, 0, 6 );
}

IPAddress AWSNetwork::cidr_to_mask( byte cidr )
{
	uint32_t		subnet;

	if ( cidr )
		subnet = htonl( ~(( 1 << ( 32 - cidr )) -1 ) );
	else
		subnet = htonl( 0 );

	return IPAddress( subnet );
}

bool AWSNetwork::connect_to_wifi()
{
	uint8_t		remaining_attempts	= 10;
	char		*ip = nullptr;
	char		*cidr = nullptr;
	const char	*ssid		= config->get_parameter<const char *>( "wifi_sta_ssid" );
	const char	*password	= config->get_parameter<const char *>( "wifi_sta_password" );
	char		*dummy;

	if (( WiFi.status () == WL_CONNECTED ) && !strcmp( ssid, WiFi.SSID().c_str() )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Already connected to SSID [%s].\n", ssid );
		return true;
	}

	Serial.printf( "[INFO] Attempting to connect to SSID [%s] ", ssid );

	if ( static_cast<aws_ip_mode>(config->get_parameter<int>( "wifi_sta_ip_mode" )) == aws_ip_mode::fixed ) {

		if ( etl::string_view( config->get_parameter<const char *>( "wifi_sta_ip" )).size() ) {

			etl::string<32> buf;
			strlcpy( buf.data(), config->get_parameter<const char *>( "wifi_sta_ip" ), buf.capacity() );
		 	ip = strtok_r( buf.data(), "/", &dummy );
		}
		if ( ip )
			cidr = strtok_r( nullptr, "/", &dummy );

  		wifi_sta_ip.fromString( ip );
  		wifi_sta_gw.fromString( config->get_parameter<const char *>( "wifi_sta_gw" ));
  		wifi_sta_dns.fromString( config->get_parameter<const char *>( "wifi_sta_dns" ));
		// flawfinder: ignore
  		wifi_sta_subnet = cidr_to_mask( static_cast<unsigned int>( atoi( cidr ) ));
		WiFi.config( wifi_sta_ip, wifi_sta_gw, wifi_sta_subnet, wifi_sta_dns );

	}
	WiFi.begin( ssid , password );

	while (( WiFi.status() != WL_CONNECTED ) && ( --remaining_attempts > 0 )) {	// NOSONAR

    Serial.print( "." );
		delay( 1000 );
	}

	if ( WiFi.status () == WL_CONNECTED ) {

		wifi_sta_ip = WiFi.localIP();
		wifi_sta_subnet = WiFi.subnetMask();
		wifi_sta_gw = WiFi.gatewayIP();
		wifi_sta_dns = WiFi.dnsIP();
		Serial.printf( " OK. Using IP [%s]\n", WiFi.localIP().toString().c_str() );
		return true;
	}

	Serial.printf( "NOK.\n" );

	return false;
}

byte AWSNetwork::get_eth_cidr_prefix( void )
{
	return mask_to_cidr( (uint32_t)eth_subnet );
}

IPAddress *AWSNetwork::get_eth_dns( void )
{
	return &eth_dns;
}

IPAddress *AWSNetwork::get_eth_gw( void )
{
	return &eth_gw;
}

IPAddress *AWSNetwork::get_eth_ip( void )
{
	return &eth_ip;
}

byte AWSNetwork::get_wifi_sta_cidr_prefix( void )
{
	return mask_to_cidr( (uint32_t)wifi_sta_subnet );
}

IPAddress *AWSNetwork::get_wifi_sta_dns( void )
{
	return &wifi_sta_dns;
}

IPAddress *AWSNetwork::get_wifi_sta_gw( void )
{
	return &wifi_sta_gw;
}

IPAddress *AWSNetwork::get_wifi_sta_ip( void )
{
	return &wifi_sta_ip;
}

uint8_t *AWSNetwork::get_wifi_mac( void )
{
	return wifi_mac;
}

bool AWSNetwork::initialise_ethernet( void )
{
	char	*ip			= nullptr;
	char	*cidr		= nullptr;
	char	*dummy;
	uint8_t	eth_mac[6] = { 0xFE, 0xED, 0xDE, 0xAD, 0xBE, 0xEF };	// NOSONAR

	pinMode( 4, OUTPUT );
	digitalWrite( 4, HIGH );
	delay( 250 );
	digitalWrite( 4, LOW );
	delay( 50 );
	digitalWrite( 4, HIGH );
	delay( 350 );

	ESP32_W5500_onEvent();

	// You will have to change esp_w5500.cpp ETH.begin() to make it accept eth_mac even if a mac already exists,
	// honestly I do not understand why you would silently override the eth_mac parameter
	if ( !ETH.begin( GPIO_SPI_MISO, GPIO_SPI_MOSI, GPIO_SPI_SCK, GPIO_SPI_CS_ETH, GPIO_SPI_INT, SPI_CLOCK_MHZ, SPI3_HOST, eth_mac )) {

		Serial.printf( "[ERROR] Could not initialise ethernet!\n" );
		return false;
	}

  	if ( static_cast<aws_ip_mode>( config->get_parameter<int>( "eth_ip_mode" )) == aws_ip_mode::fixed ) {

		if ( etl::string_view( config->get_parameter<const char *>( "eth_ip" )).size() ) {

			etl::string<32> buf;
			strlcpy( buf.data(), config->get_parameter<const char *>( "eth_ip" ), buf.capacity() );
		 	ip = strtok_r( buf.data(), "/", &dummy );
		}
		if ( ip )
			cidr = strtok_r( nullptr, "/", &dummy );

  		eth_ip.fromString( ip );
  		eth_gw.fromString( config->get_parameter<const char *>( "eth_gw" ));
  		eth_dns.fromString( config->get_parameter<const char *>( "eth_dns" ));
		// flawfinder: ignore
		ETH.config( eth_ip, eth_gw, cidr_to_mask( static_cast<unsigned int>( atoi( cidr )) ), eth_dns );
  	}

	ESP32_W5500_waitForConnect();

	eth_ip = ETH.localIP();
	eth_gw = ETH.gatewayIP();
	eth_subnet = ETH.subnetMask();
	eth_dns = ETH.dnsIP();

	return true;
}

bool AWSNetwork::initialise( AWSConfig *_config, bool _debug_mode )
{
	debug_mode = _debug_mode;
	config = _config;
	
	esp_read_mac( wifi_mac, ESP_MAC_WIFI_STA );

	switch ( static_cast<aws_iface>( config->get_parameter<int>( "pref_iface" )) ) {

		case aws_iface::wifi_ap:
		case aws_iface::wifi_sta:
			return initialise_wifi();

		case aws_iface::eth:
			return initialise_ethernet();

		default:
			Serial.printf( "[ERROR] Invalid preferred iface, falling back to WiFi.\n" );
			initialise_wifi();
			return false;
	}
	return false;
}

bool AWSNetwork::initialise_wifi( void )
{
	switch ( static_cast<aws_wifi_mode>( config->get_parameter<int>( "wifi_mode" )) ) {

		case aws_wifi_mode::ap:
			if ( debug_mode )
				Serial.printf( "[DEBUG] Booting in AP mode.\n" );
			WiFi.mode( WIFI_AP );
			return start_hotspot();

		case aws_wifi_mode::both:
			if ( debug_mode )
				Serial.printf( "[DEBUG] Booting in AP+STA mode.\n" );
			WiFi.mode( WIFI_AP_STA );
			return ( start_hotspot() && connect_to_wifi() );

		case aws_wifi_mode::sta:
			if ( debug_mode )
				Serial.printf( "[DEBUG] Booting in STA mode.\n" );
			WiFi.mode( WIFI_STA );
			return connect_to_wifi();

		default:
			Serial.printf( "[ERROR] Unknown wifi mode [%d]\n",  config->get_parameter<int>( "wifi_mode" ) );
	}
	return false;
}

byte AWSNetwork::mask_to_cidr( uint32_t subnet )
{
	byte		cidr 	= 0;
	uint32_t	mask 	= 0x80000000;
	uint32_t	x 		= ntohl( subnet );

	while ( mask != 0 ) {
		if (( x & mask ) != mask )
			break;
		cidr++;
		mask >>= 1;
	}
	return cidr;
}

bool AWSNetwork::post_content( const char *endpoint, size_t endpoint_len, const char *jsonString )
{
	uint8_t				fe_len;
	etl::string<128>	final_endpoint;
	HTTPClient 			http;
	int 				l;
	const char			*remote_server = config->get_parameter<const char *>( "remote_server" );
	bool				sent = false;
	uint8_t				status;
	etl::string<128>	url;
	uint8_t 			url_len = 8 + etl::string_view( config->get_parameter<const char *>( "remote_server" )).size() + 1 + etl::string_view( config->get_parameter<const char *>( "url_path" )).size() + 1;
	const char			*url_path = config->get_parameter<const char *>( "url_path" );
	WiFiClientSecure	wifi_client;

	if ( url_len > 128 )
		url.resize( url_len );
		
	 l = snprintf( url.data(), url_len, "https://%s/%s/", remote_server, url_path );

	if ( debug_mode )
		Serial.printf( "[DEBUG] Connecting to server [%s:443] ...", remote_server );

	fe_len = l + endpoint_len + 2;
	if ( fe_len > 128 )
		final_endpoint.resize( fe_len );
		
	final_endpoint.assign( url );
	final_endpoint.append( endpoint );

	// FIXME: factorise code
	if ( static_cast<aws_iface>( config->get_parameter<int>( "pref_iface" )) == aws_iface::eth ) {

	return false;
	//	wifi_client.setCACert( config->get_root_ca().data() );
		if ( !wifi_client.connect( remote_server, 443 )) {

			if ( debug_mode )
        		Serial.print( "NOK.\n" );

		} else {

			if ( debug_mode )
        		Serial.print( "OK.\n" );
		}
				
//http.begin( wifi_client, final_endpoint.data() );

    if ( debug_mode )
				Serial.printf( "OK.\n" );

			http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
			http.addHeader( "Host", remote_server );
			http.addHeader( "Accept", "application/json" );
			http.addHeader( "Content-Type", "application/json" );
			Serial.printf("ETH HTTPS POST STRING [%s]\n", jsonString );
			status = http.POST( jsonString );
			if ( debug_mode ) {

				Serial.print( "[DEBUG] HTTP response: " );
				Serial.printf( "%d\n", status );
			}
			http.end();
			    	wifi_client.stop();

			if ( status == 200 )
				sent = true;

	} else {

		wifi_client.setCACert( config->get_root_ca().data() );
		if ( !wifi_client.connect( remote_server, 443 )) {

			if ( debug_mode )
        		Serial.print( "NOK.\n" );

		} else {

			if ( debug_mode )
        		Serial.print( "OK.\n" );
		}

		http.begin( wifi_client, final_endpoint.data() );
		http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
		http.addHeader( "Content-Type", "application/json" );
		status = http.POST( jsonString );
		if ( debug_mode ) {

			Serial.print( "[DEBUG] HTTP response: " );
			Serial.printf( "%d\n", status );
		}
		if ( status== 200 )
			sent = true;

		http.end();
    	wifi_client.stop();
	}

	return sent;
}

bool AWSNetwork::start_hotspot( void )
{
	const char	*ssid		= config->get_parameter<const char *>( "wifi_ap_ssid" );
	const char	*password	= config->get_parameter<const char *>( "wifi_ap_password" );

	char *ip = nullptr;
	char *cidr = nullptr;
	char *dummy;

	if ( debug_mode )
		Serial.printf( "[DEBUG] Trying to start AP on SSID [%s] with password [%s]\n", ssid, password );

	if ( WiFi.softAP( ssid, password )) {

		if ( etl::string_view( config->get_parameter<const char *>( "wifi_ap_ip" )).size() ) {

			etl::string<32> buf;
			strlcpy( buf.data(), config->get_parameter<const char *>( "wifi_ap_ip" ), buf.capacity() );
		 	ip = strtok_r( buf.data(), "/", &dummy );
		}
		if ( ip )
			cidr = strtok_r( nullptr, "/", &dummy );

  		wifi_ap_ip.fromString( ip );
  		wifi_ap_gw.fromString( config->get_parameter<const char *>( "wifi_ap_gw" ));
		// flawfinder: ignore
  		wifi_ap_subnet = cidr_to_mask( static_cast<unsigned int>( atoi( cidr )) );

		WiFi.softAPConfig( wifi_ap_ip, wifi_ap_gw, wifi_ap_subnet );
		Serial.printf( "[INFO] Started hotspot on SSID [%s/%s] and configuration server @ IP=%s/%s\n", ssid, password, ip, cidr );
		return true;
	}
	return false;
}

bool AWSNetwork::stop_hotspot( void )
{
	return WiFi.softAPdisconnect();
}
