/*
  	AWS.cpp

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
#include <ESP32OTAPull.h>
#include <ESPping.h>
#include <stdio.h>
#include <stdarg.h>
#include <FS.h>
#include <SPIFFS.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "AstroWeatherStation.h"
#include "AWSSensorManager.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "dome.h"
#include "alpaca.h"
#include "AWS.h"

extern void IRAM_ATTR		_handle_rain_event( void );

extern SemaphoreHandle_t	sensors_read_mutex;		// FIXME: hide this within the sensor manager

const std::array<std::string, 3> pwr_mode_str = { "Solar panel", "12V DC", "PoE" };

const bool				FORMAT_SPIFFS_IF_FAILED = true;
const unsigned long		CONFIG_MODE_GUARD		= 5000000;	// 5 seconds

RTC_DATA_ATTR time_t 	rain_event_timestamp = 0;
RTC_DATA_ATTR time_t 	boot_timestamp = 0;
RTC_DATA_ATTR time_t 	last_ntp_time = 0;
RTC_DATA_ATTR uint16_t	ntp_time_misses = 0;
RTC_DATA_ATTR bool		catch_rain_event = false;

AstroWeatherStation::AstroWeatherStation( void )
{
	alpaca = nullptr;
	config = new AWSConfig();
	config_mode = false;
	debug_mode = false;
	dome = nullptr;
	ntp_synced = false;
	ota_board = nullptr;
	ota_config = nullptr;
	ota_device = nullptr;
	rain_event = false;
	sc16is750 = nullptr;
	sensor_manager = nullptr;
	server = nullptr;
	solar_panel = false;
	uptime[ 0 ] = 0;
}

void AstroWeatherStation::check_ota_updates( void )
{
	ESP32OTAPull	ota;
	// flawfinder: ignore
	char			string[64];
	int				ret_code;
	uint8_t			*wifi_mac = network.get_wifi_mac();

	snprintf( string, 64, "%s_%d", ESP.getChipModel(), ESP.getChipRevision() );
	ota.OverrideBoard( string );
	snprintf( string, 64, "%02x:%02x:%02x:%02x:%02x:%02x", wifi_mac[0], wifi_mac[1], wifi_mac[2], wifi_mac[3], wifi_mac[4], wifi_mac[5] );
	ota.OverrideDevice( string );
	snprintf( string, 64, "%d-%s-%s-%s", config->get_pwr_mode(), config->get_pcb_version(), REV, BUILD_DATE );
	ota.SetConfig( string );
	ota.SetCallback( OTA_callback );
	Serial.printf( "[INFO] Checking for OTA firmware update.\n" );
	ret_code = ota.CheckForOTAUpdate( "https://www.datamancers.net/images/AWS.json", REV );
	Serial.printf( "[INFO] Firmware OTA update result: %s.\n", OTA_message( ret_code ));
}

void AstroWeatherStation::check_rain_event_guard_time( void )
{
	time_t now;

	if ( catch_rain_event )
		return;

	if ( ntp_synced )
		time( &now );
	else
		now = last_ntp_time + ( US_SLEEP / 1000000 ) * ntp_time_misses;

	if ( ( now - rain_event_timestamp ) <= config->get_rain_event_guard_time() )
		return;

	if ( config->get_has_rain_sensor() ) {

		if ( debug_mode )
			Serial.printf( "\n[DEBUG] Rain event guard time elapsed.\n" );

		pinMode( GPIO_RAIN_SENSOR_RAIN, INPUT );
		if ( !solar_panel )
			attachInterrupt( GPIO_RAIN_SENSOR_RAIN, _handle_rain_event, FALLING );
		catch_rain_event = true;
	}
}

void AstroWeatherStation::display_banner()
{
	uint8_t	*wifi_mac = network.get_wifi_mac();
	
	Serial.printf( "\n#############################################################################################\n" );
	Serial.printf( "# AstroWeatherStation                                                                       #\n" );
	Serial.printf( "#  (c) Lesage Franck - lesage@datamancers.net                                               #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# HARDWARE SETUP                                                                            #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# MCU               : Model %s Revision %d", ESP.getChipModel(), ESP.getChipRevision() );
	print_config_string( "# WIFI Mac          : %02x:%02x:%02x:%02x:%02x:%02x", wifi_mac[0], wifi_mac[1], wifi_mac[2], wifi_mac[3], wifi_mac[4], wifi_mac[5] );
	print_config_string( "# Power Mode        : %s", pwr_mode_str[ static_cast<byte>( config->get_pwr_mode() ) ].c_str() );
	print_config_string( "# PCB version       : %s", config->get_pcb_version() );
	print_config_string( "# Ethernet present  : %s", config->get_has_ethernet() ? "Yes" : "No" );
	print_config_string( "# GPIO ext. present : %s", config->get_has_sc16is750() ? "Yes" : "No" );
	print_config_string( "# Firmware          : %s-%s", REV, BUILD_DATE );

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

const char *AstroWeatherStation::get_anemometer_sensorname( void )
{
	if ( config->get_has_ws() )
		return config->get_anemometer_model_str();
	return "N/A";
}

uint16_t AstroWeatherStation::get_config_port( void )
{
	return config->get_config_port();
}

bool AstroWeatherStation::get_debug_mode( void )
{
	return debug_mode;
}

AWSDome	*AstroWeatherStation::get_dome( void )
{
	return dome;
}

byte AstroWeatherStation::get_eth_cidr_prefix( void )
{
	return network.get_eth_cidr_prefix();
}

bool AstroWeatherStation::get_location_coordinates( double *latitude, double *longitude )
{
	if ( sensor_manager->get_sensor_data()->gps.fix ) {

		*longitude = sensor_manager->get_sensor_data()->gps.longitude;
		*latitude = sensor_manager->get_sensor_data()->gps.latitude;
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

char *AstroWeatherStation::get_json_sensor_data( void )
{
	DynamicJsonDocument	json_data(768);
	sensor_data_t		*sensor_data = sensor_manager->get_sensor_data();

	json_data["battery_level"] = sensor_data->battery_level;
	json_data["timestamp"] = sensor_data->timestamp;
	json_data["rain_event"] = sensor_data->rain_event;
	json_data["temperature"] = sensor_data->temperature;
	json_data["pressure"] = sensor_data->pressure;
	json_data["sl_pressure"] = sensor_data->sl_pressure;
	json_data["rh"] = sensor_data->rh;
	json_data["ota_board"] = ota_board;
	json_data["ota_device"] = ota_device;
	json_data["ota_config"] = ota_config;
	json_data["wind_speed"] = sensor_data->wind_speed;
	json_data["wind_gust"] = sensor_data->wind_gust;
	json_data["wind_direction"] = sensor_data->wind_direction;
	json_data["dew_point"] = sensor_data->dew_point;
	json_data["rain_intensity"] = sensor_data->rain_intensity;
	json_data["sky_temperature"] = sensor_data->sky_temperature;
	json_data["ambient_temperature"] = sensor_data->ambient_temperature;
	json_data["msas"] = sensor_data->msas;
	json_data["nelm"] = sensor_data->nelm;
	json_data["integration_time"] = sensor_data->integration_time;
	json_data["gain"] = sensor_data->gain;
	json_data["ir_luminosity"] = sensor_data->ir_luminosity;
	json_data["full_luminosity"] = sensor_data->full_luminosity;
	json_data["lux"] = sensor_data->lux;
	json_data["irradiance"] = sensor_data->irradiance;
	json_data["ntp_time_sec"] = sensor_data->ntp_time.tv_sec;
	json_data["ntp_time_usec"] = sensor_data->ntp_time.tv_usec;
	json_data["gps_fix"] = sensor_data->gps.fix;
	json_data["gps_longitude"] = sensor_data->gps.longitude;
	json_data["gps_latitude"] = sensor_data->gps.latitude;
	json_data["gps_altitude"] = sensor_data->gps.altitude;
	json_data["gps_time_sec"] = sensor_data->gps.time.tv_sec;
	json_data["gps_time_usec"] = sensor_data->gps.time.tv_usec;
	json_data["uptime"] = get_uptime();

	serializeJson( json_data, json_sensor_data, DATA_JSON_STRING_MAXLEN );
	return json_sensor_data;
}

char *AstroWeatherStation::get_json_string_config( void )
{
	return config->get_json_string_config();
}

char* AstroWeatherStation::get_root_ca( void )
{
	return config->get_root_ca();
}

sensor_data_t *AstroWeatherStation::get_sensor_data( void )
{
	return sensor_manager->get_sensor_data();
}

char *AstroWeatherStation::get_uptime( void )
{
	int64_t uptime_s;

	if ( !on_solar_panel() )

		uptime_s = round( esp_timer_get_time() / 1000000 );

 	else {

		time_t now;
		time( &now );
		if ( !boot_timestamp ) {

			uptime_s = round( esp_timer_get_time() / 1000000 );
			boot_timestamp = now - uptime_s;

		} else

			uptime_s = now - boot_timestamp;
	}

	int days = floor( uptime_s / ( 3600 * 24 ));
	int hours = floor( fmod( uptime_s, 3600 * 24 ) / 3600 );
	int minutes = floor( fmod( uptime_s, 3600 ) / 60 );
	int seconds = fmod( uptime_s, 60 );

	snprintf( uptime, 32, "%03dd:%02dh:%02dm:%02ds", days, hours, minutes, seconds );
	return uptime;
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

const char	*AstroWeatherStation::get_wind_vane_sensorname( void )
{
	if ( config->get_has_wv() )
		return config->get_wind_vane_model_str();
	return "N/A";
}

void AstroWeatherStation::handle_rain_event( void )
{
	if ( solar_panel ) {

		sensor_manager->read_rain_sensor();
		send_rain_event_alarm( sensor_manager->rain_intensity_str() );

	} else

		detachInterrupt( GPIO_RAIN_SENSOR_RAIN );

	time( &rain_event_timestamp );
	sensor_manager->set_rain_event();
	if ( config->get_has_dome() && config->get_close_dome_on_rain() )
		dome->trigger_close();
	catch_rain_event = false;
}

bool AstroWeatherStation::has_gps( void )
{
	return config->get_has_gps();
}

bool AstroWeatherStation::has_rain_sensor( void )
{
	return config->get_has_rain_sensor();
}

bool AstroWeatherStation::initialise( void )
{
	unsigned long	start = micros();
	unsigned long	guard = 0;
	// flawfinder: ignore
	char			string[64];
	uint8_t			mac[6];

 	pinMode( GPIO_DEBUG, INPUT );

	debug_mode = static_cast<bool>( 1 - gpio_get_level( GPIO_DEBUG )) || DEBUG_MODE;
	while ( (( micros() - start ) <= CONFIG_MODE_GUARD ) && !( gpio_get_level( GPIO_DEBUG ))) {

		delay( 100 );
		guard = micros() - start;
	}

	config_mode = ( guard > CONFIG_MODE_GUARD );

	Serial.printf( "\n\n[INFO] AstroWeatherStation [REV %s, BUILD %s] is booting...\n", REV, BUILD_DATE );

	if ( !config->load( debug_mode ) )
		return false;

	solar_panel = ( config->get_pwr_mode() == aws_pwr_src::panel );
	snprintf( string, 64, "%s_%d", ESP.getChipModel(), ESP.getChipRevision() );
	ota_board = strdup( string );

	if ( ( esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED ) && solar_panel )
		boot_timestamp = 0;

	esp_read_mac( mac, ESP_MAC_WIFI_STA );
	snprintf( string, 64, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
	ota_device = strdup( string );

	snprintf( string, 64, "%d-%s-%s-%s",config->get_pwr_mode(), config->get_pcb_version(), REV, BUILD_DATE );
	ota_config = strdup( string );

	// Initialise here and read battery level before initialising WiFi as GPIO_13 is on ADC_2, and is not available when WiFi is on.
	sensor_manager = new AWSSensorManager( solar_panel, debug_mode );

	if ( solar_panel )
		sensor_manager->read_battery_level();

	// The idea is that if we did something stupid with the config and the previous setup was correct, we can restore it, otherwise, move on ...
	if ( !network.initialise( config, debug_mode ) && config->can_rollback() )
		return config->rollback();

	if ( config->get_has_sc16is750() ) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Initialising SC16IS750.\n" );

		sc16is750 = new I2C_SC16IS750( DEFAULT_SC16IS750_ADDR );
	}

	if ( solar_panel ) {

		//FIXME: we already get this to cater for station uptime
		esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
		rain_event = ( ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason );

		if ( config_mode )
			enter_config_mode();

	} else

		start_config_server();

	if ( !startup_sanity_check() && config->can_rollback() )
		return config->rollback();

	if ( debug_mode )
		display_banner();

	// Do not enable earlier as some HW configs rely on SC16IS750 to pilot the dome.
	if ( config->get_has_dome() ) {
		if ( config->get_has_sc16is750() )
			dome = new AWSDome( sc16is750, sensor_manager->get_i2c_mutex(), debug_mode );
		else
			dome = new AWSDome( debug_mode );

	}

	if ( solar_panel ) {

		sync_time();
		check_rain_event_guard_time();
	}

	if ( rain_event ) {

		sensor_manager->initialise( sc16is750, config, rain_event );
		handle_rain_event();
		return true;
	}

	if ( !sensor_manager->initialise( sc16is750, config, false ))
		return false;

	if ( !solar_panel ) {

		alpaca = new alpaca_server( debug_mode );
		switch ( config->get_alpaca_iface() ) {

			case aws_iface::wifi_sta:
				alpaca->start( WiFi.localIP() );
				break;

			case aws_iface::wifi_ap:
				alpaca->start( WiFi.softAPIP() );
				break;

			case aws_iface::eth:
				alpaca->start( Ethernet.localIP() );
				break;

		}

		if ( config->get_has_rain_sensor() ) {

			if ( debug_mode )
				Serial.printf( "[DEBUG] Now monitoring rain event pin from rain sensor\n" );

			pinMode( GPIO_RAIN_SENSOR_RAIN, INPUT );
			attachInterrupt( GPIO_RAIN_SENSOR_RAIN, _handle_rain_event, FALLING );
		}

		std::function<void(void *)> _feed = std::bind( &AstroWeatherStation::periodic_tasks, this, std::placeholders::_1 );
		xTaskCreatePinnedToCore(
			[](void *param) {
				std::function<void(void*)>* periodic_tasks_proxy = static_cast<std::function<void(void*)>*>( param );
				(*periodic_tasks_proxy)( NULL );
			}, "GPSFeed", 2000, &_feed, 5, &aws_periodic_task_handle, 1 );
	}

	return true;
}

void AstroWeatherStation::initialise_sensors( void )
{
	sensor_manager->initialise_sensors( sc16is750 );
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
	return ( sensor_manager->get_available_sensors() & sensor_id );
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

void AstroWeatherStation::periodic_tasks( void *dummy )
{
	uint8_t	sync_time_mod = 1;

	while ( true ) {

		if ( sync_time_mod % 5 )

			sync_time_mod++;

		else {

			sync_time();
			sync_time_mod = 1;
		}
		check_rain_event_guard_time();
		delay( 1000 );
	}
}

bool AstroWeatherStation::poll_sensors( void )
{
	return sensor_manager->poll_sensors();
}

void AstroWeatherStation::print_config_string( const char *fmt, ... )
{
	// flawfinder: ignore
  	char	string[96];
	byte 	i;
	va_list	args;

	memset( string, 0, 96 );
	va_start( args, fmt );
	// flawfinder: ignore
	int l = vsnprintf( string, 92, fmt, args );	// NOSONAR
	va_end( args );
	if ( l >= 0 ) {
		for( i = l; i < 92; string[ i++ ] = ' ' );
		strlcat( string, "#\n", 95 );
	}
	Serial.printf( "%s", string );
}

void AstroWeatherStation::print_runtime_config( void )
{
	// flawfinder: ignore
  	char	string[97];
	char	*root_ca = config->get_root_ca();
	int		ca_pos = 0;

	print_config_string( "# AP SSID      : %s", config->get_wifi_ap_ssid() );
	print_config_string( "# AP PASSWORD  : %s", config->get_wifi_ap_password() );
	print_config_string( "# AP IP        : %s", config->get_wifi_ap_ip() );
	print_config_string( "# AP Gateway   : %s", config->get_wifi_ap_gw() );
	print_config_string( "# STA SSID     : %s", config->get_wifi_sta_ssid() );
	print_config_string( "# STA PASSWORD : %s", config->get_wifi_sta_password() );
	print_config_string( "# STA IP       : %s", config->get_wifi_sta_ip() );
	print_config_string( "# STA Gateway  : %s", config->get_wifi_sta_gw() );
	print_config_string( "# SERVER       : %s", config->get_remote_server() );
	print_config_string( "# URL PATH     : /%s", config->get_url_path() );
	print_config_string( "# TZNAME       : %s", config->get_tzname() );

	memset( string, 0, 97 );
	int str_len = snprintf( string, 96, "# ROOT CA      : " );
	// flawfinder: ignore
	int ca_len = strlen( root_ca );
	int string_pos;
	while ( ca_pos < ca_len ) {

		strlcat( string, root_ca + ca_pos, 92 );
		for ( string_pos = str_len; string_pos < 92; string_pos++ ) {

			if ( string[ string_pos ] == '\n' ) {

				if (( ca_pos < 92 ) || (( ca_len - ca_pos ) < 92 ))

					string[ string_pos ] = ' ';

				else  {

					memcpy( string + string_pos, root_ca + ca_pos + 1, 96 - string_pos - 3 );
					ca_pos++;
				}
			}

			ca_pos++;
			if ( ca_pos > ca_len )
				break;
		}
		ca_pos--;
		for( int j = string_pos; j < 92; string[ j++ ] = ' ' );
		memset( string + 91, 0, 6 );
		strlcat( string, " #\n", 96 );
		Serial.printf( "%s", string );
		memset( string, 0, 97 );
		str_len = snprintf( string, 96, "# " );
	}

	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# SENSORS & CONTROLS                                                                        #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# DOME             : %s", config->get_has_dome() ? "Yes" : "No" );
	print_config_string( "# GPS              : %s", config->get_has_gps() ? "Yes" : "No" );
	print_config_string( "# SQM/IRRANDIANCE  : %s", config->get_has_tsl() ? "Yes" : "No" );
	print_config_string( "# CLOUD SENSOR     : %s", config->get_has_mlx() ? "Yes" : "No" );
	print_config_string( "# RH/TEMP/PRES.    : %s", config->get_has_bme() ? "Yes" : "No" );
	print_config_string( "# WINDVANE         : %s", config->get_has_wv() ? "Yes" : "No" );
	print_config_string( "# WINDVANE MODEL   : %s", config->get_wind_vane_model_str() );
	print_config_string( "# ANEMOMETER       : %s", config->get_has_ws() ? "Yes" : "No" );
	print_config_string( "# ANEMOMETER MODEL : %s", config->get_anemometer_model_str() );
	print_config_string( "# RAIN SENSOR      : %s", config->get_has_rain_sensor() ? "Yes" : "No" );

}

bool AstroWeatherStation::rain_sensor_available( void )
{
	return sensor_manager->rain_sensor_available();
}

void AstroWeatherStation::read_sensors( void )
{
	sensor_manager->read_sensors();
}

void AstroWeatherStation::reboot( void )
{
	ESP.restart();
}

void AstroWeatherStation::report_unavailable_sensors( void )
{
	// flawfinder: ignore
	const char	sensor_name[7][13] = { "MLX96014 ", "TSL2591 ", "BME280 ", "WIND VANE ", "ANEMOMETER ", "RAIN_SENSOR ", "GPS " };
	// flawfinder: ignore
	char		unavailable_sensors[96] = "Unavailable sensors: ";
	uint8_t		j = sensor_manager->get_available_sensors();
	uint8_t		k;

	k = j;
	for ( uint8_t i = 0; i < 7; i++ ) {

		if ( !( k & 1 ))
			strlcat( unavailable_sensors, sensor_name[i], 13 );
		k >>= 1;
	}

	if ( debug_mode ) {

		Serial.printf( "[DEBUG] %s", unavailable_sensors );

		if ( j == ALL_SENSORS )
			Serial.printf( "none.\n" );
	}

	if ( j != ALL_SENSORS )
		send_alarm( "Unavailable sensors report", unavailable_sensors );
}

void AstroWeatherStation::send_alarm( const char *subject, const char *message )
{
	DynamicJsonDocument content( 512 );
	// flawfinder: ignore
	char jsonString[ 600 ];
	content["subject"] = subject;
	content["message"] = message;

	serializeJson( content, jsonString );
	network.post_content( "alarm.php", jsonString );
}

void AstroWeatherStation::send_backlog_data( void )
{
	// flawfinder: ignore
	char	line[1024];
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

		int	i = backlog.readBytesUntil( '\n', line, DATA_JSON_STRING_MAXLEN - 1 );
		if ( !i )
			break;
		line[i] = '\0';
		if ( !network.post_content( "newData.php",  line )) {

			empty = false;
			// flawfinder: ignore
			if ( new_backlog.printf( "%s\n", line ) == ( 1 + strlen( line )) ) {

				if ( debug_mode )
					Serial.printf( "[DEBUG] Data saved in backlog.\n" );

			} else

				Serial.printf( "[ERROR] Could not write data into the backlog.\n" );

		} else

			if ( debug_mode )
				Serial.printf( "[DEBUG] Sucessfully sent backlog data: %s\n", line );

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
    	Serial.printf( "[DEBUG] Sensor data: %s\n", json_sensor_data );

	if ( network.post_content( "newData.php",  json_sensor_data ))
		send_backlog_data();
	else
		store_unsent_data( json_sensor_data );

	if ( !solar_panel )
		xSemaphoreGive( sensors_read_mutex );
}

void AstroWeatherStation::send_rain_event_alarm( const char *str )
{
	// flawfinder: ignore
	char		msg[32];

	snprintf( msg, 32, "RAIN! Level = %s", str );
	send_alarm( "Rain event", msg );
}

bool AstroWeatherStation::start_config_server( void )
{
	server = new AWSWebServer();
	server->initialise( debug_mode );
	return true;
}

bool AstroWeatherStation::startup_sanity_check( void )
{
	switch ( config->get_pref_iface() ) {

		case aws_iface::wifi_sta:
			return Ping.ping( WiFi.gatewayIP(), 3 );

		case aws_iface::eth:
			return Ping.ping( ETH.gatewayIP(), 3 );

		case aws_iface::wifi_ap:
			return true;
	}
	return false;
}

bool AstroWeatherStation::store_unsent_data( char *data )
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
	if (( ok = ( backlog.printf( "%s\n", data ) == ( strlen( data ) + 1 )) )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Ok, data secured for when server is available. data=%s\n", data );

	} else

		Serial.printf( "[ERROR] Could not write data.\n" );

	backlog.close();
	return ok;
}

bool AstroWeatherStation::sync_time( void )
{
	const char	*ntp_server = "pool.ntp.org";
	uint8_t		ntp_retries = 5;
   	struct 		tm timeinfo;

	if ( config->get_has_gps() && sensor_manager->get_sensor_data()->gps.fix )
		return true;

	if ( debug_mode )
		Serial.printf( "[DEBUG] Connecting to NTP server " );

	configTzTime( config->get_tzname(), ntp_server );

	while ( !( ntp_synced = getLocalTime( &timeinfo )) && ( --ntp_retries > 0 ) ) {

		if ( debug_mode )
			Serial.printf( "." );
		delay( 1000 );
		configTzTime( config->get_tzname(), ntp_server );
	}
	if ( debug_mode ) {

		Serial.printf( "\n[DEBUG] %sNTP Synchronised. ", ntp_synced ? "" : "NOT " );
		Serial.print( "Time and date: " );
		Serial.print( &timeinfo, "%Y-%m-%d %H:%M:%S\n" );
		Serial.printf( "\n" );
	}

	if ( ntp_synced ) {

		time( &sensor_manager->get_sensor_data()->timestamp );
		sensor_manager->get_sensor_data()->ntp_time.tv_sec = sensor_manager->get_sensor_data()->timestamp;
		if ( ntp_time_misses )
			ntp_time_misses = 0;
		last_ntp_time = sensor_manager->get_sensor_data()->timestamp;

	} else {

		// Not proud of this but it should be sufficient if the number of times we miss ntp sync is not too big
		ntp_time_misses++;
		sensor_manager->get_sensor_data()->timestamp =  last_ntp_time + ( US_SLEEP / 1000000 ) * ntp_time_misses;

	}
	return ntp_synced;
}

bool AstroWeatherStation::update_config( JsonVariant &proposed_config )
{
	return config->save_runtime_configuration( proposed_config );
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
	const char	*ssid		= config->get_wifi_sta_ssid();
	const char	*password	= config->get_wifi_sta_password();
	char		*dummy;

	if (( WiFi.status () == WL_CONNECTED ) && !strcmp( ssid, WiFi.SSID().c_str() )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Already connected to SSID [%s].\n", ssid );
		return true;
	}

	Serial.printf( "[INFO] Attempting to connect to SSID [%s] ", ssid );

	if ( config->get_wifi_sta_ip_mode() == aws_ip_mode::fixed ) {

		if ( config->get_wifi_sta_ip() ) {

			// flawfinder: ignore
			char buf[32];
			strlcpy( buf, config->get_wifi_sta_ip(), 32 );
		 	ip = strtok_r( buf, "/", &dummy );
		}
		if ( ip )
			cidr = strtok_r( nullptr, "/", &dummy );

  		wifi_sta_ip.fromString( ip );
  		wifi_sta_gw.fromString( config->get_wifi_sta_gw() );
  		wifi_sta_dns.fromString( config->get_wifi_sta_dns() );
		// flawfinder: ignore
  		wifi_sta_subnet = cidr_to_mask( static_cast<unsigned int>( atoi( cidr ) ));
		WiFi.config( wifi_sta_ip, wifi_sta_gw, wifi_sta_subnet, wifi_sta_dns );

	}
	WiFi.begin( ssid , password );

	while (( WiFi.status() != WL_CONNECTED ) && ( --remaining_attempts > 0 )) {

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

bool AWSNetwork::disconnect_from_wifi( void )
{
	byte i = 0;

	WiFi.disconnect();
	while ( (i++ < 20) && ( WiFi.status() == WL_CONNECTED ))
    	delay( 100 );

	return ( WiFi.status() != WL_CONNECTED );
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
	char *ip = nullptr;
	char *cidr = nullptr;
	char *dummy;
	uint8_t	eth_mac[6] = { 0xFE, 0xED, 0xDE, 0xAD, 0xBE, 0xEF };

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

  	if ( config->get_eth_ip_mode() == aws_ip_mode::fixed ) {

		if ( config->get_eth_ip() ) {

			// flawfinder: ignore
			char buf[32];
			strlcpy( buf, config->get_eth_ip(), 32 );
		 	ip = strtok_r( buf, "/", &dummy );
		}
		if ( ip )
			cidr = strtok_r( nullptr, "/", &dummy );

  		eth_ip.fromString( ip );
  		eth_gw.fromString( config->get_eth_gw() );
  		eth_dns.fromString( config->get_eth_dns() );
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

	switch ( config->get_pref_iface() ) {

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
	switch ( config->get_wifi_mode() ) {

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
			Serial.printf( "[ERROR] Unknown wifi mode [%d]\n", config->get_wifi_mode() );
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

bool AWSNetwork::post_content( const char *endpoint, const char *jsonString )
{
	const char	*remote_server = config->get_remote_server();
	const char	*url_path = config->get_url_path();
	char		*url;
	bool		sent = false;

	WiFiClientSecure wifi_client;
	HTTPClient http;

	uint8_t status;
	// flawfinder: ignore
	uint8_t url_len = 7 + strlen( remote_server ) + 1 + strlen( url_path ) + 3;
	uint8_t fe_len;

	char *final_endpoint;

	url = static_cast<char *>( malloc( url_len ) );
	int l = snprintf( url, url_len, "https://%s/%s/", remote_server, url_path );

	if ( debug_mode )
		Serial.printf( "[DEBUG] Connecting to server [%s:443] ...", remote_server );

	// flawfinder: ignore
	fe_len = l + strlen( endpoint ) + 2;
	final_endpoint = static_cast<char *>( malloc( fe_len ) );
	strlcpy( final_endpoint, url, fe_len );
	strlcat( final_endpoint, endpoint, fe_len );

	// FIXME: factorise code
	if ( config->get_pref_iface() == aws_iface::eth ) {

		if ( http.begin( final_endpoint )) {

			if ( debug_mode )
				Serial.printf( "OK.\n" );

			http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
			http.addHeader( "Host", remote_server );
			http.addHeader( "Accept", "application/json" );
			http.addHeader( "Content-Type", "application/json" );
			status = http.POST( jsonString );
			if ( debug_mode ) {

				Serial.print( "[DEBUG] HTTP response: " );
				Serial.printf( "%d\n", status );
			}
			http.end();
			if ( status == 200 )
				sent = true;

		} else
			if ( debug_mode )
				Serial.printf( "NOK.\n" );

	} else {

		wifi_client.setCACert( config->get_root_ca() );
		if ( !wifi_client.connect( remote_server, 443 )) {

			if ( debug_mode )
        		Serial.print( "NOK.\n" );

		} else {

			if ( debug_mode )
        		Serial.print( "OK.\n" );
		}

		http.begin( wifi_client, final_endpoint );
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

	free( final_endpoint );
	free( url );
	return sent;
}

bool AWSNetwork::shutdown_wifi( void )
{
	if ( debug_mode )
		Serial.printf( "[DEBUG] Shutting down WIFI.\n" );

	if ( !stop_hotspot() || !disconnect_from_wifi() )
		return false;

	WiFi.mode( WIFI_OFF );
	return true;
}

bool AWSNetwork::start_hotspot( void )
{
	const char	*ssid		= config->get_wifi_ap_ssid();
	const char	*password	= config->get_wifi_ap_password();

	char *ip = nullptr;
	char *cidr = nullptr;
	char *dummy;

	if ( debug_mode )
		Serial.printf( "[DEBUG] Trying to start AP on SSID [%s] with password [%s]\n", ssid, password );

	if ( WiFi.softAP( ssid, password )) {

		if ( config->get_wifi_ap_ip() ) {

			// flawfinder: ignore
			char buf[32];
			strlcpy( buf, config->get_wifi_ap_ip(), 32 );
		 	ip = strtok_r( buf, "/", &dummy );
		}
		if ( ip )
			cidr = strtok_r( nullptr, "/", &dummy );

  		wifi_ap_ip.fromString( ip );
  		wifi_ap_gw.fromString( config->get_wifi_ap_gw() );
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
