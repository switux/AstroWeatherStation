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

#include <rom/rtc.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include <Preferences.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <thread>
#include <stdio.h>
#include <stdarg.h>
#include <FS.h>
#include <LittleFS.h>
#include <charconv>
#include <optional>
#include <TinyGPSPlus.h>

#include "Embedded_Template_Library.h"
#include "etl/string.h"
#include "AWSOTA.h"

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
#include "AWSUpdater.h"
#include "AWSNetwork.h"
#include "AstroWeatherStation.h"

extern void IRAM_ATTR		_handle_rain_event( void );
extern SemaphoreHandle_t	sensors_read_mutex;

const std::array<etl::string<10>, 3> PWR_MODE_STR = { "SolarPanel", "12VDC", "PoE" };

const bool				FORMAT_LITTLEFS_IF_FAILED = true;
const unsigned long		CONFIG_MODE_GUARD		= 5000000;	// 5 seconds
const unsigned long		FACTORY_RESET_GUARD		= 15000000;	// 15 seconds

RTC_DATA_ATTR time_t 	rain_event_timestamp = 0;		// NOSONAR
RTC_DATA_ATTR time_t 	boot_timestamp = 0;				// NOSONAR
RTC_DATA_ATTR time_t 	last_ntp_time = 0;				// NOSONAR
RTC_DATA_ATTR uint16_t	ntp_time_misses = 0;			// NOSONAR
RTC_DATA_ATTR bool		catch_rain_event = false;		// NOSONAR
RTC_DATA_ATTR uint16_t 	low_battery_event_count = 0;	// NOSONAR
RTC_NOINIT_ATTR bool	ota_update_ongoing = false;		// NOSONAR

AstroWeatherStation::AstroWeatherStation( void )
{
	station_data.health.init_heap_size = xPortGetFreeHeapSize();
	station_data.health.current_heap_size = station_data.health.init_heap_size;
	station_data.health.largest_free_heap_block = heap_caps_get_largest_free_block( MALLOC_CAP_8BIT );
	location = DEFAULT_LOCATION;
}

void AstroWeatherStation::check_ota_updates( bool force_update = false )
{
	ota_status_t	ota_retcode;

	Serial.printf( "[STATION   ] [INFO ] Checking for OTA firmware update.\n" );

	ota_update_ongoing = true;
	ota.set_aws_board_id( ota_setup.board );
	ota.set_aws_device_id( ota_setup.device );
	ota.set_aws_config( ota_setup.config );
	ota.set_progress_callback( OTA_callback );
	ota_setup.last_update_ts = get_timestamp();

	ota_retcode = ota.check_for_update( config.get_parameter<const char *>( "ota_url" ), config.get_root_ca().data(), ota_setup.version, force_update ? ota_action_t::UPDATE_AND_BOOT : ota_action_t::CHECK_ONLY );
	Serial.printf( "[STATION   ] [INFO ] Firmware OTA update result: (%d) %s.\n", ota_retcode, OTA_message( ota_retcode ));
	ota_update_ongoing = false;

	updater.check_for_new_files( ota_setup.version.data(), config.get_root_ca().data(), "www.datamancers.net", "images" );
}

void AstroWeatherStation::check_rain_event_guard_time( uint16_t guard_time )
{
	time_t now = get_timestamp();

	if ( catch_rain_event )
		return;

	if ( ( now - rain_event_timestamp ) <= guard_time )
		return;

	if ( debug_mode )
		Serial.printf( "\n[STATION   ] [DEBUG] Rain event guard time elapsed.\n" );

	pinMode( GPIO_RAIN_SENSOR_RAIN, INPUT );
	if ( !solar_panel )
		attachInterrupt( GPIO_RAIN_SENSOR_RAIN, _handle_rain_event, FALLING );
	catch_rain_event = true;
}

void AstroWeatherStation::close_dome_shutter( void )
{
	lookout.suspend();
	station_devices.dome.close_shutter();
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

void AstroWeatherStation::determine_boot_mode( void )
{
 	unsigned long start					= micros();
	unsigned long button_pressed_secs	= 0;

	pinMode( GPIO_DEBUG, INPUT );

	debug_mode = static_cast<bool>( 1 - gpio_get_level( GPIO_DEBUG )) || DEBUG_MODE;
	while ( !( gpio_get_level( GPIO_DEBUG ))) {

		if (( micros() - start ) >= FACTORY_RESET_GUARD	) {
			boot_mode = aws_boot_mode_t::FACTORY_RESET;
			break;
		}

		if (( micros() - start ) >= CONFIG_MODE_GUARD )
			boot_mode = aws_boot_mode_t::MAINTENANCE;

		delay( 100 );
		button_pressed_secs = micros() - start;
	}
}

void AstroWeatherStation::display_banner()
{
	if ( !debug_mode )
		return;

	uint8_t					*wifi_mac = network.get_wifi_mac();
	int						i;
	bool					gpioext = config.get_has_device( aws_device_t::SC16IS750_DEVICE );
	bool					eth = config.get_has_device( aws_device_t::ETHERNET_DEVICE );
	std::array<uint8_t,6>	eth_mac = config.get_eth_mac();

	Serial.printf( "\n#############################################################################################\n" );
	Serial.printf( "# AstroWeatherStation                                                                       #\n" );
	Serial.printf( "#  (c) lesage@loads.ch                                                                      #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# HARDWARE SETUP                                                                            #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# Board              : %s", ota_setup.board.data() );
	print_config_string( "# Model              : %s", ota_setup.config.data() );
	print_config_string( "# WIFI Mac           : %s", ota_setup.device.data() );
	print_config_string( "# Ethernet present   : %s",  eth ? "Yes" : "No" );
	if ( eth )
		print_config_string( "# ETH Mac            : %02x:%02x:%02x:%02x:%02x:%02x", eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5] );
	print_config_string( "# GPIO ext. present  : %s", gpioext ? "Yes" : "No" );
	print_config_string( "# Firmware           : %s", ota_setup.version.data() );

	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# GPIO PIN CONFIGURATION                                                                    #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_config_string( "# Wind sensors  : RX=%d TX=%d CTRL=%d", GPIO_WIND_SENSOR_RX, GPIO_WIND_SENSOR_TX, GPIO_WIND_SENSOR_CTRL );
	print_config_string( "# Rain sensor   : RX=%d TX=%d MCLR=%d RAIN=%d", GPIO_RAIN_SENSOR_RX, GPIO_RAIN_SENSOR_TX, GPIO_RAIN_SENSOR_MCLR, GPIO_RAIN_SENSOR_RAIN );
	if ( solar_panel ) {

		print_config_string( "# 3.3V SWITCH   : %d", GPIO_ENABLE_3_3V );
		print_config_string( "# 12V SWITCH    : %d", GPIO_ENABLE_12V );
		print_config_string( "# BAT LVL       : SW=%d ADC=%d", GPIO_BAT_ADC_EN, GPIO_BAT_ADC );

	} else {

		print_config_string( "# Status LEDs   : G=%d B=%d", GPIO_LED_GREEN, GPIO_LED_BLUE  );
		print_config_string( "# Dome command  : %d", gpioext ? GPIO_DOME_1 : GPIO_DOME_1_DIRECT );
		print_config_string( "# Dome open     : %d", GPIO_DOME_OPEN );
		print_config_string( "# Dome closed   : %d", GPIO_DOME_CLOSED );
		print_config_string( "# GPS           : RX=%d TX=%d", GPS_RX,GPS_TX );
		print_config_string( "# Ethernet      : MOSI=%d MISO=%d SCK=%d CS=%d INT=%d", GPIO_SPI_MOSI, GPIO_SPI_MISO, GPIO_SPI_SCK, GPIO_SPI_CS_ETH, GPIO_SPI_INT );
	}
	print_config_string( "# DEBUG/CONFIG : %d", GPIO_DEBUG );

	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );
	Serial.printf( "# RUNTIME CONFIGURATION                                                                     #\n" );
	Serial.printf( "#-------------------------------------------------------------------------------------------#\n" );

	print_runtime_config();

	Serial.printf( "#############################################################################################\n" );
}

void AstroWeatherStation::enter_config_mode( void )
{
	if ( debug_mode )
		Serial.printf( "[STATION   ] [INFO ] Entering config mode...\n ");

	if ( !start_config_server() )
		Serial.printf( "[STATION   ] [ERROR] Failed to start WiFi AP, cannot enter config mode.\n ");
	else
		while( true );
}

void AstroWeatherStation::factory_reset( void )
{
	Serial.printf( "[STATION   ] [INFO ] Performing factory reset.\n ");
	config.factory_reset();
	reboot();
}

template<typename... Args>
etl::string<96> AstroWeatherStation::format_helper( const char *fmt, Args... args )
{
	char buf[96];	// NOSONAR
	snprintf( buf, 95, fmt, args... );
	return etl::string<96>( buf );
}

etl::string_view AstroWeatherStation::get_anemometer_sensorname( void )
{
	if ( config.get_has_device( aws_device_t::ANEMOMETER_SENSOR ) )
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

etl::string_view AstroWeatherStation::get_json_sensor_data( void )
{
	JsonDocument	json_data;
	sensor_data_t	*sensor_data = sensor_manager.get_sensor_data();
	int				l;

	json_data["available_sensors"] = static_cast<unsigned long>( sensor_data->available_sensors );
	json_data["battery_level"] = station_data.health.battery_level;
	json_data["timestamp"] = sensor_data->timestamp;
	json_data["rain_event"] = sensor_data->weather.rain_event;
	json_data["temperature"] = sensor_data->weather.temperature;
	json_data["pressure"] = sensor_data->weather.pressure;
	json_data["sl_pressure"] = sensor_data->weather.sl_pressure;
	json_data["rh"] = sensor_data->weather.rh;
	json_data["ota_board"] = ota_setup.board.data();
	json_data["ota_device"] = ota_setup.device.data();
	json_data["ota_config"] = ota_setup.config.data();
	json_data["build_id" ] = ota_setup.version.data();
	json_data["wind_speed"] = sensor_data->weather.wind_speed;
	json_data["wind_gust"] = sensor_data->weather.wind_gust;
	json_data["wind_direction"] = sensor_data->weather.wind_direction;
	json_data["dew_point"] = sensor_data->weather.dew_point;
	json_data["rain_intensity"] = sensor_data->weather.rain_intensity;
	json_data["raw_sky_temperature"] = sensor_data->weather.raw_sky_temperature;
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
	json_data["ota_code" ] = static_cast<int>( ota_setup.status_code );
	json_data["ota_status_ts" ] = ota_setup.status_ts;
	json_data["ota_last_update_ts" ] = ota_setup.last_update_ts;
	json_data["reset_reason"] = station_data.reset_reason;
	json_data["shutter_status"] = static_cast<int>( station_data.dome_data.shutter_status );
	json_data["shutter_closed"] = station_data.dome_data.closed_sensor;
	json_data["shutter_open"] = station_data.dome_data.open_sensor;
	json_data["shutter_close"] = station_data.dome_data.close_command;
	json_data["lookout_active"] = lookout.is_active();

	if ( ( l = measureJson( json_data )) > json_sensor_data.capacity() ) {

		etl::string<64> tmp;
		snprintf( tmp.data(), tmp.capacity(), "sensor_data json is too small ( %d > %d )", l, json_sensor_data.capacity() );
		send_alarm( "[STATION] BUG", tmp.data() );
		Serial.printf( "[STATION   ] [BUG  ] sensor_data json is too small ( %d > %d ). Please report to support!\n", l, json_sensor_data.capacity() );
		return etl::string_view( "" );

	}
	json_sensor_data_len = serializeJson( json_data, json_sensor_data.data(), json_sensor_data.capacity() );
	if ( debug_mode )
		Serial.printf( "[STATION   ] [DEBUG] sensor_data is %d bytes long, max size is %d bytes.\n", json_sensor_data_len, json_sensor_data.capacity() );

	return etl::string_view( json_sensor_data );
}

etl::string_view AstroWeatherStation::get_json_string_config( void )
{
	return etl::string_view( config.get_json_string_config() );
}

etl::string_view AstroWeatherStation::get_location( void )
{
	return etl::string_view( location );
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

etl::string_view AstroWeatherStation::get_lookout_rules_state_json_string( void )
{
	return lookout.get_rules_state();
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

time_t AstroWeatherStation::get_timestamp( void )
{
	time_t now = 0;

	if ( ntp_synced || ( config.get_has_device( aws_device_t::GPS_SENSOR ) && station_data.gps.fix ))
		time( &now );

	return now;
}

etl::string_view AstroWeatherStation::get_unique_build_id( void )
{
	return etl::string_view( ota_setup.version );
}

uint32_t AstroWeatherStation::get_uptime( void )
{
	compute_uptime();
	return station_data.health.uptime;
}

etl::string_view AstroWeatherStation::get_wind_vane_sensorname( void )
{
	if ( config.get_has_device( aws_device_t::WIND_VANE_SENSOR ))
		return etl::string_view( config.get_wind_vane_model_str() );
	return etl::string_view( "N/A" );
}

void AstroWeatherStation::handle_event( aws_event_t event )
{
	switch( event ) {

		case aws_event_t::DOME_SHUTTER_OPEN_CHANGE:
			station_devices.dome.shutter_open_change();
			break;

		case aws_event_t::RAIN:
			if ( solar_panel ) {

				sensor_manager.read_rain_sensor();
				send_rain_event_alarm( sensor_manager.rain_intensity_str() );

			} else
				lookout.set_rain_event();

			sensor_manager.set_rain_event();
			break;

		case aws_event_t::DOME_SHUTTER_CLOSED_CHANGE:
			station_devices.dome.shutter_closed_change();
			break;

		default:
			break;
	}
}

bool AstroWeatherStation::has_device( aws_device_t device )
{
	switch( device ) {
		case aws_device_t::MLX_SENSOR:
		case aws_device_t::GPS_SENSOR:
		case aws_device_t::RAIN_SENSOR:
			return config.get_has_device( device );
		default:
			return false;
	}
}

bool AstroWeatherStation::initialise( void )
{
	std::array<uint8_t,6>	mac;
	byte					offset = 0;
	std::array<uint8_t,32>	sha_256;

	determine_boot_mode();

	pinMode( GPIO_LED_GREEN, OUTPUT );
	pinMode( GPIO_LED_BLUE, OUTPUT );
	std::function<void(void *)> _led_task = std::bind( &AstroWeatherStation::led_task, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
			std::function<void(void*)>* led_task_proxy = static_cast<std::function<void(void*)>*>( param );	// NOSONAR
			(*led_task_proxy)( NULL );
		}, "AWSLedTask", 2000, &_led_task, 10, &aws_led_task_handle, 1 );

	set_led_status( station_status_t::BOOTING );

    esp_partition_get_sha256( esp_ota_get_running_partition(), sha_256.data() );
	for ( uint8_t _byte : sha_256 ) {

		etl::string<3> h;
		snprintf( h.data(), 3, "%02x", _byte );
		station_data.firmware_sha56 += h.data();
    }

	Serial.printf( "\n\n[STATION   ] [INFO ] Firmware checksum = [%s]\n", station_data.firmware_sha56.data() );
	Serial.printf( "[STATION   ] [INFO ] AstroWeatherStation [REV %s, BUILD %s, BASE %s] is booting...\n", REV.data(), BUILD_ID, GITHASH );

	station_data.reset_reason = esp_reset_reason();

	if ( boot_mode == aws_boot_mode_t::FACTORY_RESET )
		factory_reset();

	if ( !config.load( debug_mode ) ) {

		set_led_status( station_status_t::CONFIG_ERROR );
		return false;
	}

	station_data.health.fs_free_space = config.get_fs_free_space();
	Serial.printf( "[STATION   ] [INFO ] Free space on config partition: %d bytes\n", station_data.health.fs_free_space );

	solar_panel = ( static_cast<aws_pwr_src>( config.get_pwr_mode()) == aws_pwr_src::panel );
	if ( solar_panel )
		setCpuFrequencyMhz( 80 );

	sensor_manager.set_solar_panel( solar_panel );
	sensor_manager.set_debug_mode( debug_mode );

	if ( ( esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED ) && solar_panel )
		boot_timestamp = 0;

	ota_setup.board = "AWS_";
	ota_setup.board += ESP.getChipModel();

	esp_read_mac( mac.data(), ESP_MAC_WIFI_STA );
	snprintf( ota_setup.device.data(), ota_setup.device.capacity(), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );

	ota_setup.config = PWR_MODE_STR[ static_cast<int>( config.get_pwr_mode()) ];
	ota_setup.config += "_";
	ota_setup.config += config.get_pcb_version().data();

	ota_setup.version = REV;
	ota_setup.version += ".";
	ota_setup.version += BUILD_ID;

	read_battery_level();

	set_led_status( station_status_t::NETWORK_INIT );

	// The idea is that if we did something stupid with the config and the previous setup was correct, we can restore it, otherwise, move on ...
	if ( !network.initialise( &config, debug_mode )) {

		if ( !config.rollback() ) {

			set_led_status( station_status_t::NETWORK_ERROR );
			return false;
		}
		reboot();
	}

	if ( config.get_parameter<bool>( "automatic_updates" ) && ( !solar_panel || ( station_data.health.battery_level > 50 )))
		check_ota_updates( true );

	if ( ota_update_ongoing ) {

		Serial.printf( "[STATION   ] [INFO ] Expected OTA firmware sha256 is [%s]\n", config.get_ota_sha256().data() );
		Serial.printf( "[STATION   ] [INFO ] Finalising OTA update by checking for new file package.\n" );
		updater.check_for_new_files( ota_setup.version.data(), config.get_root_ca().data(), "www.datamancers.net", "images" );
		ota_update_ongoing = false;

	}

	if ( solar_panel ) {

		// Issue #143
		esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
		rain_event = ( ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason );

		if ( boot_mode == aws_boot_mode_t::MAINTENANCE )
			enter_config_mode();

	} else

		start_config_server();

	if ( !startup_sanity_check() && config.can_rollback() ) {

		if ( !config.rollback() ) {

			set_led_status( station_status_t::NETWORK_ERROR );
			return false;
		}
		reboot();
	}

	display_banner();

	// Do not enable earlier as some HW configs rely on SC16IS750 to pilot the dome.
	initialise_dome();

	if ( solar_panel ) {

		sync_time( true );
		if ( config.get_has_device( aws_device_t::RAIN_SENSOR ))
			check_rain_event_guard_time( config.get_parameter<int>( "rain_event_guard_time" ) );
	}

	if ( rain_event ) {

		sensor_manager.initialise( &station_devices.sc16is750, &config, rain_event );
		handle_event( aws_event_t::RAIN );
		return true;
	}

	if ( config.get_has_device( aws_device_t::GPS_SENSOR ) )
		initialise_GPS();

	if ( !sensor_manager.initialise( &station_devices.sc16is750, &config, false )) {

		set_led_status( station_status_t::SENSOR_INIT_ERROR );
		return false;
	}

	if ( solar_panel )
		return true;

	start_alpaca_server();

	if ( config.get_has_device( aws_device_t::RAIN_SENSOR ) ) {

		Serial.printf( "[STATION   ] [INFO ] Monitoring rain sensor for rain event.\n" );
		pinMode( GPIO_RAIN_SENSOR_RAIN, INPUT );
		attachInterrupt( GPIO_RAIN_SENSOR_RAIN, _handle_rain_event, FALLING );
	}

	if ( config.get_parameter<bool>( "lookout_enabled" ))
		lookout.initialise( &config, &sensor_manager, &station_devices.dome, debug_mode );
	else
		if ( config.get_has_device( aws_device_t::DOME_DEVICE ))
			station_devices.dome.close_shutter();	// Issue #144

	std::function<void(void *)> _periodic_tasks = std::bind( &AstroWeatherStation::periodic_tasks, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
			std::function<void(void*)>* periodic_tasks_proxy = static_cast<std::function<void(void*)>*>( param );	// NOSONAR
			(*periodic_tasks_proxy)( NULL );
		}, "AWSCoreTask", 6000, &_periodic_tasks, 5, &aws_periodic_task_handle, 1 );


	ready = true;
	set_led_status( station_status_t::READY );
	return true;
}

void AstroWeatherStation::initialise_dome( void )
{
	if ( config.get_has_device( aws_device_t::DOME_DEVICE ) ) {

		if ( config.get_has_device( aws_device_t::SC16IS750_DEVICE ) )
			station_devices.dome.initialise( &station_devices.sc16is750, sensor_manager.get_i2c_mutex(), &station_data.dome_data, debug_mode );
		else
			station_devices.dome.initialise( &station_data.dome_data, debug_mode );
	}
}

void AstroWeatherStation::initialise_GPS( void )
{
	if ( debug_mode )
		Serial.printf( "[STATION   ] [DEBUG] Initialising GPS.\n" );

	if ( ( config.get_has_device( aws_device_t::SC16IS750_DEVICE ) && !station_devices.gps.initialise( &station_data.gps, &station_devices.sc16is750, sensor_manager.get_i2c_mutex() )) || !station_devices.gps.initialise( &station_data.gps )) {

			Serial.printf( "[STATION   ] [ERROR] GPS initialisation failed.\n" );
			return;
	}
	station_devices.gps.start();
	station_devices.gps.pilot_rtc( true );
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

bool AstroWeatherStation::is_ready( void )
{
	return ready;
}

bool AstroWeatherStation::issafe( void )
{
	if ( config.get_parameter<bool>( "lookout_enabled" ))
		return lookout.issafe();

	if ( sensor_manager.sensor_is_available( aws_device_t::RAIN_SENSOR ) && !sensor_manager.get_sensor_data()->weather.rain_event )
		return true;

	return false;
}

bool AstroWeatherStation::is_sensor_initialised( aws_device_t sensor_id )
{
	return (( sensor_manager.get_available_sensors() & sensor_id ) == sensor_id );
}

void AstroWeatherStation::led_task( void *dummy )
{
	while( true ) {

		switch( led_status ) {

			case station_status_t::BOOTING:

				analogWrite( GPIO_LED_GREEN, 0 );
				analogWrite( GPIO_LED_BLUE, 0 );
				delay( 100 );
				break;

			case station_status_t::READY:

				analogWrite( GPIO_LED_GREEN, 127 );
				analogWrite( GPIO_LED_BLUE, 0 );
				delay( 1000 );
				break;

			case station_status_t::CONFIG_ERROR:

				analogWrite( GPIO_LED_GREEN, 255 );
				analogWrite( GPIO_LED_BLUE, 255 );
				delay( 1000 );
				break;

			case station_status_t::NETWORK_INIT:

				analogWrite( GPIO_LED_BLUE, 255 );
				analogWrite( GPIO_LED_GREEN, 0 );
				delay( 500 );
				analogWrite( GPIO_LED_BLUE, 0 );
				delay( 500 );
				break;

			case station_status_t::NETWORK_ERROR:

				analogWrite( GPIO_LED_BLUE, 255 );
				analogWrite( GPIO_LED_GREEN, 0 );
				delay( 1000 );
				analogWrite( GPIO_LED_BLUE, 0 );
				delay( 1000 );
				break;

			case station_status_t::SENSOR_INIT_ERROR:

				analogWrite( GPIO_LED_GREEN, 255 );
				analogWrite( GPIO_LED_BLUE, 0 );
				delay( 1000 );
				analogWrite( GPIO_LED_GREEN, 0 );
				delay( 1000 );
				break;

			case station_status_t::OTA_UPGRADE:

				analogWrite( GPIO_LED_GREEN, 255 );
				analogWrite( GPIO_LED_BLUE, 0 );
				delay( 500 );
				analogWrite( GPIO_LED_GREEN, 0 );
				delay( 500 );
				break;

			default:
				delay( 100 );

		}
	}
}
bool AstroWeatherStation::on_solar_panel( void )
{
	return solar_panel;
}

void AstroWeatherStation::open_dome_shutter( void )
{
	lookout.suspend();
	station_devices.dome.open_shutter();
}

void OTA_callback( int offset, int total_length )
{
	static float	percentage = 0.F;
	float			p = ( 100.F * offset / total_length );

	if ( p - percentage > 10.F ) {
		esp_task_wdt_reset();
		Serial.printf("[STATION   ] [INFO ] Updating %d of %d (%02d%%)...\n", offset, total_length, 100 * offset / total_length );
		percentage = p;
	}
	esp_task_wdt_reset();
}

const char *AstroWeatherStation::OTA_message( ota_status_t code )
{
	ota_setup.status_code = code;
	switch ( code ) {

		case ota_status_t::UPDATE_AVAILABLE:
			return "An update is available but wasn't installed";

		case ota_status_t::NO_UPDATE_PROFILE_FOUND:
			return "No profile matches";

		case ota_status_t::NO_UPDATE_AVAILABLE:
			return "Profile matched, but update not applicable";

		case ota_status_t::UPDATE_OK:
			return "An update was done, but no reboot";

		case ota_status_t::HTTP_FAILED:
			return "HTTP GET failure";

		case ota_status_t::WRITE_ERROR:
			return "Write error";

		case ota_status_t::JSON_PROBLEM:
			return "Invalid JSON";

		case ota_status_t::OTA_UPDATE_FAIL:
			return "Update failure (no OTA partition?)";

		case ota_status_t::CONFIG_ERROR:
			return "OTA config has a problem";

		case ota_status_t::UNKNOWN:
			return "Undefined status";

	}
	return "Unhandled OTA status code";
}

void AstroWeatherStation::periodic_tasks( void *dummy )	// NOSONAR
{
	unsigned long	sync_time_millis			= 0;
	unsigned long	sync_time_timer				= 5000;
	unsigned long	ota_millis					= 0;
	unsigned long	ota_timer					= 30 * 60 * 1000;
	unsigned long	data_push_millis			= 0;
	int				data_push_timer				= config.get_parameter<int>( "push_freq" );
	uint16_t		rain_event_guard_time		= config.get_parameter<int>( "rain_event_guard_time" );
	bool			auto_ota_updates			= config.get_parameter<bool>( "automatic_updates" );

	if ( !config.get_parameter<bool>( "data_push" ))
		data_push_timer = 0;

	while ( true ) {

		if (( millis() - sync_time_millis ) > sync_time_timer ) {

			station_data.health.current_heap_size =	xPortGetFreeHeapSize();
			station_data.health.largest_free_heap_block = heap_caps_get_largest_free_block( MALLOC_CAP_8BIT );
			sync_time( false );
			sync_time_millis = millis();
		}

		if ( force_ota_update || ( auto_ota_updates && (( millis() - ota_millis ) > ota_timer ))) {

			force_ota_update = false;
			check_ota_updates( true );
			ota_millis = millis();
		}

		if ( config.get_has_device( aws_device_t::RAIN_SENSOR ))
			check_rain_event_guard_time( rain_event_guard_time );

		if ( data_push_timer && (( millis() - data_push_millis ) > 1000*data_push_timer )) {

			send_data();
			data_push_millis = millis();
		}

		delay( 500 );
	}
}

bool AstroWeatherStation::poll_sensors( void )
{
	if ( config.get_has_device( aws_device_t::GPS_SENSOR ) )
		read_GPS();
	return sensor_manager.poll_sensors();
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

void AstroWeatherStation::print_runtime_config( void )
{
 	std::array<char,97>	string;
	const char			*root_ca	= config.get_root_ca().data();
	int					ca_pos		= 0;
	IPAddress			ipv4;
	char				*ip			= nullptr;
	char				*cidr		= nullptr;
	char				*dummy;
	uint8_t				_cidr;
	etl::string<32>		buf;

	strlcpy( buf.data(), config.get_parameter<const char *>( "eth_ip" ), buf.capacity() );
	ip = strtok_r( buf.data(), "/", &dummy );
	if ( ip ) {
		cidr = strtok_r( nullptr, "/", &dummy );
		ipv4.fromString( ip );
	}
	if (( ipv4 == network.get_ip( aws_iface::eth ) ) && ( atoi( cidr ) == WiFiGenericClass::calculateSubnetCIDR( network.get_subnet( aws_iface::eth ) )))
		print_config_string( "# ETH IP       : %s", config.get_parameter<const char *>( "eth_ip" ));
	else
		print_config_string( "# ETH IP       : %s/%d", network.get_ip( aws_iface::eth ).toString(), WiFiGenericClass::calculateSubnetCIDR( network.get_subnet( aws_iface::eth ) ) );

	ipv4.fromString( config.get_parameter<const char *>( "eth_gw" ) );
	if ( ipv4 == network.get_gw( aws_iface::eth ) )
		print_config_string( "# ETH Gateway  : %s", config.get_parameter<const char *>( "eth_gw" ));
	else
		print_config_string( "# ETH Gateway  : %s", network.get_gw( aws_iface::eth ).toString() );

	print_config_string( "# AP SSID      : %s", config.get_parameter<const char *>( "wifi_ap_ssid" ));
	print_config_string( "# AP PASSWORD  : %s", config.get_parameter<const char *>( "wifi_ap_password" ));

	print_config_string( "# AP IP        : %s", config.get_parameter<const char *>( "wifi_ap_ip" ));
	print_config_string( "# AP Gateway   : %s", config.get_parameter<const char *>( "wifi_ap_gw" ));
	print_config_string( "# STA SSID     : %s", config.get_parameter<const char *>( "wifi_sta_ssid" ));
	print_config_string( "# STA PASSWORD : %s", config.get_parameter<const char *>( "wifi_sta_password" ));

	strlcpy( buf.data(), config.get_parameter<const char *>( "wifi_sta_ip" ), buf.capacity() );
	ip = strtok_r( buf.data(), "/", &dummy );
	if ( ip ) {
		cidr = strtok_r( nullptr, "/", &dummy );
		ipv4.fromString( ip );
	}
	if (( ipv4 == network.get_ip( aws_iface::wifi_sta ) ) && ( atoi( cidr ) == WiFiGenericClass::calculateSubnetCIDR( network.get_subnet( aws_iface::wifi_sta ) )))
		print_config_string( "# STA IP       : %s", config.get_parameter<const char *>( "wifi_sta_ip" ));
	else
		print_config_string( "# STA IP       : %s/%d", network.get_ip( aws_iface::wifi_sta ).toString(), WiFiGenericClass::calculateSubnetCIDR( network.get_subnet( aws_iface::wifi_sta ) ) );

	ipv4.fromString( config.get_parameter<const char *>( "wifi_sta_gw" ) );
	if ( ipv4 == network.get_gw( aws_iface::wifi_sta ) )
		print_config_string( "# STA Gateway  : %s", config.get_parameter<const char *>( "wifi_sta_gw" ));
	else
		print_config_string( "# STA Gateway  : %s", network.get_gw( aws_iface::wifi_sta ).toString() );

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

	print_config_string( "# DOME             : %s", config.get_has_device( aws_device_t::DOME_DEVICE ) ? "Yes" : "No" );
	print_config_string( "# GPS              : %s", config.get_has_device( aws_device_t::GPS_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# SQM/IRRADIANCE   : %s", config.get_has_device( aws_device_t::TSL_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# CLOUD SENSOR     : %s", config.get_has_device( aws_device_t::MLX_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# RH/TEMP/PRES.    : %s", config.get_has_device( aws_device_t::BME_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# WINDVANE         : %s", config.get_has_device( aws_device_t::WIND_VANE_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# WINDVANE MODEL   : %s", config.get_wind_vane_model_str().data() );
	print_config_string( "# ANEMOMETER       : %s", config.get_has_device( aws_device_t::ANEMOMETER_SENSOR ) ? "Yes" : "No" );
	print_config_string( "# ANEMOMETER MODEL : %s", config.get_anemometer_model_str().data() );
	print_config_string( "# RAIN SENSOR      : %s", config.get_has_device( aws_device_t::RAIN_SENSOR ) ? "Yes" : "No" );

}

void AstroWeatherStation::read_battery_level( void )
{
	if ( !solar_panel )
		return;

	int		adc_value = 0;

	WiFi.mode ( WIFI_OFF );

	if ( debug_mode )
		Serial.print( "[STATION   ] [DEBUG] Battery level: " );

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
			Serial.printf( "[STATION] [DEBUG] GPS FIX. LAT=%f LON=%f ALT=%f DATETIME=%s\n", station_data.gps.latitude, station_data.gps.longitude, station_data.gps.altitude, buf.data() );
		}

	} else

		if ( debug_mode )
			Serial.printf( "[STATION] [DEBUG] NO GPS FIX.\n" );
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


bool AstroWeatherStation::rain_sensor_available( void )
{
	return sensor_manager.rain_sensor_available();
}

void AstroWeatherStation::read_sensors( void )
{
	sensor_manager.read_sensors();

	if ( station_data.health.battery_level <= BAT_LEVEL_MIN ) {

		etl::string<64> string;
		snprintf( string.data(), string.capacity(), "LOW Battery level = %03.2f%%\n", station_data.health.battery_level );

		// Deal with ADC output accuracy, no need to stress about it, a few warnings are enough to get the message through :-)
		if (( low_battery_event_count >= LOW_BATTERY_COUNT_MIN ) && ( low_battery_event_count <= LOW_BATTERY_COUNT_MAX ))
			send_alarm( "Low battery", string.data() );

		low_battery_event_count++;
		if ( debug_mode )
			Serial.printf( "[CORE      ] [DEBUG] %s", string.data() );
	}
}

void AstroWeatherStation::reboot( void )
{
	ESP.restart();
}

void AstroWeatherStation::report_unavailable_sensors( void )
{
	std::array<std::string, 7>	sensor_name			= { "MLX96014 ", "TSL2591 ", "BME280 ", "WIND VANE ", "ANEMOMETER ", "RAIN_SENSOR ", "GPS " };
	std::array<char,96>			unavailable_sensors;
	aws_device_t				j					= sensor_manager.get_available_sensors();
	unsigned long				k;

	strlcpy( unavailable_sensors.data(), "Unavailable sensors: ", unavailable_sensors.size() );

	k = static_cast<unsigned long>( j );
	for ( uint8_t i = 0; i < 7; i++ ) {

		if ( ( k & aws_device_t::MLX_SENSOR ) == aws_device_t::NO_SENSOR )		// Flaky, MLX_SENSOR = 1, maybe need to rework this ....
			strlcat( unavailable_sensors.data(), sensor_name[i].c_str(), 13 );
		k >>= 1;
	}

	if ( debug_mode ) {

		Serial.printf( "[STATION   ] [DEBUG] %s", unavailable_sensors.data() );

		if ( j == ALL_SENSORS )
			Serial.printf( "none.\n" );
	}

	if ( j != ALL_SENSORS )
		send_alarm( "[STATION   ] Unavailable sensors report", unavailable_sensors.data() );
}

bool AstroWeatherStation::resume_lookout( void )
{
	return lookout.resume();
}

void AstroWeatherStation::send_alarm( const char *subject, const char *message )
{
	JsonDocument content;
	// flawfinder: ignore
	char jsonString[ 600 ];	// NOSONAR
	content["subject"] = subject;
	content["message"] = message;

	serializeJson( content, jsonString );
	network.post_content( "alarm.php", strlen( "alarm.php" ), jsonString );
	content.clear();
	content["username"] = "TestStation";
	content["content"] = message;
	serializeJson( content, jsonString );
	network.webhook( jsonString );
}

void AstroWeatherStation::send_backlog_data( void )
{
	etl::string<1024> line;
	bool	empty = true;

	LittleFS.begin( FORMAT_LITTLEFS_IF_FAILED );

	if ( !LittleFS.exists( "/unsent.txt" )) {

		if ( debug_mode )
			Serial.printf( "[STATION   ] [DEBUG] No backlog file.\n" );
		return;

	}
	// flawfinder: ignore
	File backlog = LittleFS.open( "/unsent.txt", FILE_READ );

	if ( !backlog ) {

		if ( debug_mode )
			Serial.printf( "[STATION   ] [DEBUG] No backlog data to send.\n" );
		return;
	}

	if ( !backlog.size() ) {

		backlog.close();

		if ( debug_mode )
			Serial.printf( "[STATION   ] [DEBUG] No backlog data to send.\n" );
		return;
	}

	// flawfinder: ignore
	File new_backlog = LittleFS.open( "/unsent.new", FILE_WRITE );

	while ( backlog.available() ) {

		int	i = backlog.readBytesUntil( '\n', line.data(), line.capacity() - 1 );
		if ( !i )
			break;
		line[i] = '\0';
		if ( !network.post_content( "newData.php", strlen( "newData.php" ), line.data() )) {

			empty = false;
			// flawfinder: ignore
			if ( new_backlog.printf( "%s\n", line.data() ) != ( 1 + line.size() ))
				Serial.printf( "[STATION   ] [ERROR] Could not write data into the backlog.\n" );

		}
	}

	new_backlog.close();
	backlog.close();

	if ( !empty ) {

		LittleFS.remove( "/unsent.txt" );
		LittleFS.rename( "/unsent.new", "/unsent.txt" );
		if ( debug_mode )
			Serial.printf( "[STATION   ] [DEBUG] Data backlog is not empty.\n" );

	} else {

		LittleFS.remove( "/unsent.txt" );
		LittleFS.remove( "/unsent.new" );
		if ( debug_mode )
			Serial.printf( "[STATION   ] [DEBUG] Data backlog is empty.\n");
	}
}

void AstroWeatherStation::send_data( void )
{
	if ( !solar_panel ) {

		while ( xSemaphoreTake( sensors_read_mutex, 5000 /  portTICK_PERIOD_MS ) != pdTRUE )

			if ( debug_mode )
				Serial.printf( "[STATION   ] [DEBUG] Waiting for sensor data update to complete.\n" );

	}

	get_json_sensor_data();

	if ( debug_mode )
    	Serial.printf( "[STATION   ] [DEBUG] Sensor data: %s\n", json_sensor_data.data() );

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

void AstroWeatherStation::set_led_status( station_status_t x )
{
	led_status = x;
	if ( debug_mode )
		Serial.printf( "[STATION   ] [DEBUG] LED STATUS=%d\n",static_cast<int>(x) );
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
			return Ping.ping( Ethernet.gatewayIP(), 3 );

		case aws_iface::wifi_ap:
			return true;

	}
	return false;
}

bool AstroWeatherStation::store_unsent_data( etl::string_view data )
{
	bool ok;

	// Issue #154
	LittleFS.begin( FORMAT_LITTLEFS_IF_FAILED );

	// flawfinder: ignore
	File backlog = LittleFS.open( "/unsent.txt", FILE_WRITE );

	if ( !backlog ) {

		Serial.printf( "[STATION   ] [ERROR] Cannot store data until server is available.\n" );
		return false;
	}

	if (( ok = ( backlog.printf( "%s\n", data.data()) == ( 1 + json_sensor_data_len )) )) {

		if ( debug_mode )
			Serial.printf( "[STATION   ] [DEBUG] Ok, data secured for when server is available. data=%s\n", data.data() );

	} else

		Serial.printf( "[STATION   ] [ERROR] Could not write data.\n" );

	backlog.close();
	return ok;
}

bool AstroWeatherStation::suspend_lookout( void )
{
	return lookout.suspend();
}

bool AstroWeatherStation::sync_time( bool verbose )
{
	const char	*ntp_server = "pool.ntp.org";
	uint8_t		ntp_retries = 5;
   	struct 		tm timeinfo;

	if ( config.get_has_device( aws_device_t::GPS_SENSOR ) && station_data.gps.fix ) {

		time( &sensor_manager.get_sensor_data()->timestamp );
 		return true;
	}

	if ( debug_mode && verbose )
		Serial.printf( "[STATION   ] [DEBUG] Connecting to NTP server " );

	configTzTime( config.get_parameter<const char *>( "tzname" ), ntp_server );

	while ( !( ntp_synced = getLocalTime( &timeinfo )) && ( --ntp_retries > 0 ) ) {	// NOSONAR

		if ( debug_mode && verbose )
			Serial.printf( "." );
		delay( 1000 );
		configTzTime( config.get_parameter<const char *>( "tzname" ), ntp_server );
	}
	if ( debug_mode && verbose ) {

		Serial.printf( "\n[STATION   ] [DEBUG] %sNTP Synchronised. ", ntp_synced ? "" : "NOT " );
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

void AstroWeatherStation::trigger_ota_update( void )
{
	force_ota_update = true;
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
