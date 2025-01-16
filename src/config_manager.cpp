/*
  	config_manager.cpp

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

#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>

#include "defaults.h"
#include "common.h"
#include "config_manager.h"
#include "AstroWeatherStation.h"

extern HardwareSerial Serial1;	// NOSONAR


extern AstroWeatherStation station;
extern const std::array<std::string, 3> ANEMOMETER_MODEL;

RTC_DATA_ATTR char _can_rollback = 0;	// NOSONAR

bool AWSConfig::can_rollback( void )
{
	return _can_rollback;
}

void AWSConfig::factory_reset( void )
{
	bool x;

	if ( !LittleFS.begin( true )) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Could not access flash filesystem, bailing out!\n" );
		return;
	}

	x = LittleFS.remove( "/aws.conf" );
	if ( debug_mode )
		Serial.printf( "[CONFIGMNGR] [ERROR] Config file %ssuccessfully deleted.\n", x ? "":"un" );

	LittleFS.end();
}

etl::string_view AWSConfig::get_anemometer_model_str( void )
{
	return etl::string_view( Anemometer::ANEMOMETER_MODEL[ get_parameter<int>( "anemometer_model" ) ].c_str() );
}

std::array<uint8_t,6> AWSConfig::get_eth_mac( void )
{
	return eth_mac;
}

uint32_t AWSConfig::get_fs_free_space( void )
{
	return fs_free_space;
}

etl::string_view AWSConfig::get_pcb_version( void )
{
	return etl::string_view( pcb_version );
}

aws_pwr_src	AWSConfig::get_pwr_mode( void )
{
	return pwr_mode;
}

etl::string_view AWSConfig::get_root_ca( void )
{
	return etl::string_view( root_ca );
}

bool AWSConfig::get_has_device( aws_device_t dev )
{
	return (( devices & dev ) == dev );
}

etl::string_view AWSConfig::get_json_string_config( void )
{
	static etl::string<5120>	json_string;
	int							i;

	if ( ( i = serializeJson( json_config, json_string.data(), json_string.capacity() )) >= json_string.capacity() ) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Reached configuration string limit (%d > 1024). Please contact support\n", i );
		return etl::string_view( "" );
	}
	return etl::string_view( json_string.data() );
}

etl::string_view AWSConfig::get_ota_sha256( void )
{
	return etl::string_view( ota_sha256 );
}

etl::string_view AWSConfig::get_wind_vane_model_str( void )
{
	return etl::string_view( Wind_vane::WIND_VANE_MODEL[ get_parameter<int>( "wind_vane_model" ) ].c_str() );
}

void AWSConfig::list_files( void )
{
	File root = LittleFS.open( "/" );
	File file = root.openNextFile();

	while( file ) {

      Serial.printf( "[CONFIGMNGR] [DEBUG] Filename: %05d /%s\n", file.size(), file.name() );
      file = root.openNextFile();
	}
	close( root );
}

bool AWSConfig::load( bool _debug_mode  )
{
	debug_mode = _debug_mode;

	if ( !LittleFS.begin( true )) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Could not access flash filesystem, bailing out!\n" );
		return false;
	}

	if ( debug_mode )
		list_files();

	update_fs_free_space();

	return read_config();
}

bool AWSConfig::read_config( void )
{
	read_hw_info_from_nvs();

	read_root_ca();

	if ( !read_file( "/aws.conf"  ) ) {

		if ( !read_file( "/aws.conf.dfl" )) {

			Serial.printf( "\n[CONFIGMNGR] [ERROR] Could not read config file.\n" );
			return false;
		}
		Serial.printf( "[CONFIGMNGR] [INFO ] Using minimal/factory config file.\n" );
	}

	devices |= aws_device_t::DOME_DEVICE * ( json_config["has_dome"].is<int>() ? json_config["has_dome"].as<int>() : DEFAULT_HAS_DOME );
	devices |= aws_device_t::BME_SENSOR * ( json_config["has_bme"].is<int>() ? json_config["has_bme"].as<int>() : DEFAULT_HAS_BME );
	devices |= aws_device_t::GPS_SENSOR * ( json_config["has_gps"].is<int>() ? json_config["has_gps"].as<int>() : DEFAULT_HAS_GPS );
	devices |= aws_device_t::MLX_SENSOR * ( json_config["has_mlx"].is<int>() ? json_config["has_mlx"].as<int>() : DEFAULT_HAS_MLX );
	devices |= aws_device_t::RAIN_SENSOR * ( json_config["has_rain_sensor"].is<int>() ? json_config["has_rain_sensor"].as<int>() : DEFAULT_HAS_RAIN_SENSOR );
	devices |= aws_device_t::TSL_SENSOR * ( json_config["has_tsl"].is<int>() ? json_config["has_tsl"].as<int>() : DEFAULT_HAS_TSL );
	devices |= aws_device_t::ANEMOMETER_SENSOR * ( json_config["has_ws"].is<int>() ? json_config["has_ws"].as<int>() : DEFAULT_HAS_WS );
	devices |= aws_device_t::WIND_VANE_SENSOR * ( json_config["has_wv"].is<int>() ? json_config["has_wv"].as<int>() : DEFAULT_HAS_WV );

	set_missing_parameters_to_default_values();

	// Add fixed hardware config

	json_config["has_ethernet"]= ( ( devices & aws_device_t::ETHERNET_DEVICE ) == aws_device_t::ETHERNET_DEVICE );
	json_config["has_rtc"]= ( ( devices & aws_device_t::RTC_DEVICE ) == aws_device_t::RTC_DEVICE );

	return true;
}

void AWSConfig::read_root_ca( void )
{
	File 	file = LittleFS.open( "/root_ca.txt", FILE_READ );
	int		s;

	if ( !file ) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Cannot read ROOT CA file. Using default CA.\n");
		s = strlen( DEFAULT_ROOT_CA );
		if ( s > root_ca.capacity() )
			Serial.printf( "[CONFIGMNGR] [BUG  ] Size of default ROOT CA is too big [%d > %d].\n", s, root_ca.capacity() );
		else
			root_ca.assign( DEFAULT_ROOT_CA );
		return;
	}

	if (!( s = file.size() )) {

		if ( debug_mode )
			Serial.printf( "[CONFIGMNGR] [DEBUG] Empty ROOT CA file. Using default CA.\n" );
		s = strlen( DEFAULT_ROOT_CA );
		if ( s > root_ca.capacity() )
			Serial.printf( "[CONFIGMNGR] [BUG  ] Size of default ROOT CA is too big [%d > %d].\n", s, root_ca.capacity() );
		else
			root_ca.assign( DEFAULT_ROOT_CA );
		return;
	}

	if ( s > root_ca.capacity() ) {

		Serial.printf( "[CONFIGMNGR] [ERROR] ROOT CA file is too big. Using default CA.\n" );
		s = strlen( DEFAULT_ROOT_CA );
		if ( s > root_ca.capacity() )
			Serial.printf( "[CONFIGMNGR] [BUG  ] Size of default ROOT CA is too big [%d > %d].\n", s, root_ca.capacity() );
		else
			root_ca.assign( DEFAULT_ROOT_CA );
		return;
	}

	file.readBytes( root_ca.data(), s );
	file.close();
	root_ca.uninitialized_resize( s );
}

bool AWSConfig::read_file( const char *filename )
{
	int					s;

	// flawfinder: ignore
	File file = LittleFS.open( filename, FILE_READ );

	if ( !file ) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Cannot read config file [%s]\n", filename );
		return false;
	}

	if (!( s = file.size() )) {

		if ( debug_mode )
			Serial.printf( "[CONFIGMNGR] [DEBUG] Empty config file [%s]\n", filename );
		return false;
	}

	if ( DeserializationError::Ok == deserializeJson( json_config, file )) {

		if ( debug_mode )
			Serial.printf( "[CONFIGMNGR] [DEBUG] Configuration is valid.\n");

		return true;
	}

	Serial.printf( "[CONFIGMNGR] [ERROR] Configuration file has been corrupted.\n");
	return false;
}

bool AWSConfig::read_hw_info_from_nvs( void )
{
	Preferences nvs;
	char		x;

	Serial.printf( "[CONFIGMNGR] [INFO ] Reading NVS values.\n" );

	nvs.begin( "firmware", true );
	nvs.getString( "sha256", ota_sha256.data(), ota_sha256.capacity() );
	nvs.end();

	nvs.begin( "aws", true );
	if ( !nvs.getString( "pcb_version", pcb_version.data(), pcb_version.capacity() )) {

		Serial.printf( "[CONFIGMNGR] [PANIC] Could not get PCB version from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	if ( static_cast<byte>(( pwr_mode = (aws_pwr_src) nvs.getChar( "pwr_mode", 127 ))) == 127 ) {

		Serial.printf( "[CONFIGMNGR] [PANIC] Could not get Power Mode from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}

#if 0
	if ( ( x = nvs.getChar( "has_sc16is750", 127 )) == 127 ) {

		Serial.printf( "[CONFIGMNGR] [PANIC] Could not get SC16IS750 presence from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	devices |= ( x == 0 ) ? aws_device_t::NO_SENSOR : aws_device_t::SC16IS750_DEVICE;
#endif

	if ( ( x = nvs.getChar( "has_ethernet", 127 )) == 127 ) {

		printf( "[CONFIGMNGR] [PANIC] Could not get has_ethernet from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	devices |= ( x == 0 ) ? aws_device_t::NO_SENSOR : aws_device_t::ETHERNET_DEVICE;
	if ( x )
		nvs.getBytes( "eth_mac", eth_mac.data(), 6 );

	nvs.end();

	return true;
}

bool AWSConfig::rollback()
{
	if ( !_can_rollback ) {

		if ( debug_mode )
			Serial.printf( "[CONFIGMNGR] [DEBUG] No configuration to rollback.\n");
		return true;

	}

	Serial.printf( "[CONFIGMNGR] [INFO ] Rolling back last submitted configuration.\n" );

	if ( !LittleFS.begin( true )) {

		Serial.printf( "[CONFMGR] [ERROR] Could not open filesystem.\n" );
		return false;
	}

	LittleFS.remove( "/aws.conf" );
	LittleFS.rename( "/aws.conf.bak", "/aws.conf" );
	Serial.printf( "[CONFIGMNGR] [INFO ] Rollback successful.\n" );
	_can_rollback = 0;
	return true;
}

bool AWSConfig::save_runtime_configuration( JsonVariant &_json_config )
{
	size_t	s;

	if ( !verify_entries( _json_config ))
		return false;

	if ( debug_mode )
		list_files();

	set_root_ca( _json_config );

	Serial.printf( "[CONFIGMNGR] [INFO ] Saving submitted configuration.\n" );

	if ( !LittleFS.begin( true )) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Could not open filesystem.\n" );
		return false;
	}

	update_fs_free_space();

	LittleFS.remove( "/aws.conf.bak.try" );
	LittleFS.rename( "/aws.conf", "/aws.conf.bak.try" );
	LittleFS.remove( "/aws.conf.try" );

	// flawfinder: ignore
	File file = LittleFS.open( "/aws.conf.try", FILE_WRITE );
	if ( !file ) {

		Serial.printf( "[CONFIGMNGR] [ERROR] Cannot write configuration file, rolling back.\n" );
		return false;
	}

	s = serializeJson( _json_config, file );
	file.close();
	if ( !s ) {

		LittleFS.remove( "/aws.conf" );
		LittleFS.remove( "/aws.conf.try" );
		LittleFS.rename( "/aws.conf.bak.try", "/aws.conf" );
		Serial.printf( "[CONFIGMNGR] [ERROR] Empty configuration file, rolling back.\n" );
		return false;

	}
	if ( s > MAX_CONFIG_FILE_SIZE ) {

		LittleFS.remove( "/aws.conf.try" );
		LittleFS.rename( "/aws.conf.bak.try", "/aws.conf" );
		Serial.printf( "[CONFIGMNGR] [ERROR] Configuration file is too big [%d bytes].\n" );
		station.send_alarm( "Configuration error", "Configuration file is too big. Not applying changes!" );
		return false;

	}

	LittleFS.remove( "/aws.conf" );

	if ( debug_mode )
		list_files();

	LittleFS.rename( "/aws.conf.try", "/aws.conf" );
	LittleFS.rename( "/aws.conf.bak.try", "/aws.conf.bak" );
	Serial.printf( "[CONFIGMNGR] [INFO ] Wrote %d bytes, configuration save successful.\n", s );

	LittleFS.remove( "/root_ca.txt.try" );
	LittleFS.rename( "/root_ca.txt", "/root_ca.txt.try" );

	file = LittleFS.open( "/root_ca.txt.try", FILE_WRITE );
	s = file.print( root_ca.data() );
	file.close();
	Serial.printf( "[CONFIGMNGR] [INFO ] Wrote %d bytes of ROOT CA.\n", s );
	LittleFS.rename( "/root_ca.txt.try", "/root_ca.txt" );

	_can_rollback = 1;
	if ( debug_mode )
		list_files();

	return true;
}

void AWSConfig::set_missing_network_parameters_to_default_values( void )
{
	if ( !json_config["wifi_ap_ssid"].is<JsonVariant>() )
		json_config["wifi_ap_ssid"] = DEFAULT_WIFI_AP_SSID;

	if ( !json_config["config_port"].is<JsonVariant>() )
		json_config["config_port"] = DEFAULT_CONFIG_PORT;

	if ( !json_config["eth_ip"].is<JsonVariant>() )
		json_config["eth_ip"] = DEFAULT_ETH_IP;

	if ( !json_config["eth_dns"].is<JsonVariant>() )
		json_config["eth_dns"] = DEFAULT_ETH_DNS;

	if ( !json_config["eth_gw"].is<JsonVariant>() )
		json_config["eth_gw"] = DEFAULT_ETH_GW;

	if ( !json_config["eth_ip_mode"].is<JsonVariant>() )
		json_config["eth_ip_mode"] = static_cast<int>( DEFAULT_ETH_IP_MODE );

	if ( !json_config["pref_iface"].is<JsonVariant>() )
		json_config["pref_iface"] = static_cast<int>( aws_iface::wifi_ap );

	if ( !json_config["remote_server"].is<JsonVariant>() )
		json_config["remote_server"] = DEFAULT_SERVER;

	if ( !json_config["wifi_sta_ssid"].is<JsonVariant>() )
		json_config["wifi_sta_ssid"] = DEFAULT_WIFI_STA_SSID;

	if ( !json_config["url_path"].is<JsonVariant>() )
		json_config["url_path"] = DEFAULT_URL_PATH;

	if ( !json_config["wifi_ap_dns"].is<JsonVariant>() )
		json_config["wifi_ap_dns"] = DEFAULT_WIFI_AP_DNS;

	if ( !json_config["wifi_ap_gw"].is<JsonVariant>() )
		json_config["wifi_ap_gw"] = DEFAULT_WIFI_AP_GW;

	if ( !json_config["wifi_ap_ip"].is<JsonVariant>() )
		json_config["wifi_ap_ip"] = DEFAULT_WIFI_AP_IP;

	if ( !json_config["wifi_ap_password"].is<JsonVariant>() )
		json_config["wifi_ap_password"] = DEFAULT_WIFI_AP_PASSWORD;

	if ( !json_config["wifi_mode"].is<JsonVariant>() )
		json_config["wifi_mode"] = static_cast<int>( DEFAULT_WIFI_MODE );

	if ( !json_config["wifi_sta_dns"].is<JsonVariant>() )
		json_config["wifi_sta_dns"] = DEFAULT_WIFI_STA_DNS;

	if ( !json_config["wifi_sta_gw"].is<JsonVariant>() )
		json_config["wifi_sta_gw"] = DEFAULT_WIFI_STA_GW;

	if ( !json_config["wifi_sta_ip"].is<JsonVariant>() )
		json_config["wifi_sta_ip"] = DEFAULT_WIFI_STA_IP;

	if ( !json_config["wifi_sta_ip_mode"].is<JsonVariant>() )
		json_config["wifi_sta_ip_mode"] = static_cast<int>( DEFAULT_WIFI_STA_IP_MODE );

	if ( !json_config["wifi_sta_password"].is<JsonVariant>() )
		json_config["wifi_sta_password"] = DEFAULT_WIFI_STA_PASSWORD;
}

void AWSConfig::set_missing_lookout_safe_parameters_to_default_values( void )
{
	if ( !json_config["safe_cloud_coverage_1_active"].is<JsonVariant>())
		json_config["safe_cloud_coverage_1_active"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_ACTIVE;

	if ( !json_config["safe_cloud_coverage_2_active"].is<JsonVariant>() )
		json_config["safe_cloud_coverage_2_active"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_ACTIVE;

	if ( !json_config["safe_cloud_coverage_1_delay"].is<JsonVariant>() )
		json_config["safe_cloud_coverage_1_delay"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_DELAY;

	if ( !json_config["safe_cloud_coverage_2_delay"].is<JsonVariant>() )
		json_config["safe_cloud_coverage_2_delay"] = DEFAULT_SAFE_CLOUD_COVERAGE_2_DELAY;

	if ( !json_config["safe_cloud_coverage_1_max"].is<JsonVariant>() )
		json_config["safe_cloud_coverage_1_max"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_MAX;

	if ( !json_config["safe_cloud_coverage_2_max"].is<JsonVariant>() )
		json_config["safe_cloud_coverage_2_max"] = DEFAULT_SAFE_CLOUD_COVERAGE_2_MAX;

	if ( !json_config["safe_rain_intensity_active"].is<JsonVariant>() )
		json_config["safe_rain_intensity_active"] = DEFAULT_SAFE_RAIN_INTENSITY_ACTIVE;

	if ( !json_config["safe_rain_intensity_delay"].is<JsonVariant>() )
		json_config["safe_rain_intensity_delay"] = DEFAULT_SAFE_RAIN_INTENSITY_DELAY;

	if ( !json_config["safe_rain_intensity_max"].is<JsonVariant>() )
		json_config["safe_rain_intensity_max"] = DEFAULT_SAFE_RAIN_INTENSITY_MAX;

	if ( !json_config["safe_wind_speed_active"].is<JsonVariant>() )
		json_config["safe_wind_speed_active"] = DEFAULT_SAFE_WIND_SPEED_ACTIVE;

	if ( !json_config["safe_wind_speed_max"].is<JsonVariant>() )
		json_config["safe_wind_speed_max"] = DEFAULT_SAFE_WIND_SPEED_MAX;

	if ( !json_config["safe_wind_speed_delay"].is<JsonVariant>() )
		json_config["safe_wind_speed_delay"] = DEFAULT_SAFE_WIND_SPEED_DELAY;
}

void AWSConfig::set_missing_lookout_unsafe_parameters_to_default_values( void )
{
	if ( !json_config["unsafe_cloud_coverage_1_active"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_1_active"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_ACTIVE;

	if ( !json_config["unsafe_cloud_coverage_2_active"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_2_active"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_ACTIVE;

	if ( !json_config["unsafe_cloud_coverage_1_delay"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_1_delay"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_DELAY;

	if ( !json_config["unsafe_cloud_coverage_2_delay"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_2_delay"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_DELAY;

	if ( !json_config["unsafe_cloud_coverage_1_max"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_1_max"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_MAX;

	if ( !json_config["unsafe_cloud_coverage_2_max"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_2_max"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_MAX;

	if ( !json_config["unsafe_cloud_coverage_1_missing"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_1_missing"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_MISSING;

	if ( !json_config["unsafe_cloud_coverage_2_missing"].is<JsonVariant>() )
		json_config["unsafe_cloud_coverage_2_missing"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_MISSING;

	if ( !json_config["unsafe_rain_event_active"].is<JsonVariant>() )
		json_config["unsafe_rain_event_active"] = DEFAULT_UNSAFE_RAIN_EVENT_ACTIVE;

	if ( !json_config["unsafe_rain_event_missing"].is<JsonVariant>() )
		json_config["unsafe_rain_event_missing"] = DEFAULT_UNSAFE_RAIN_EVENT_MISSING;

	if ( !json_config["unsafe_rain_intensity_active"].is<JsonVariant>() )
		json_config["unsafe_rain_intensity_active"] = DEFAULT_UNSAFE_RAIN_INTENSITY_ACTIVE;

	if ( !json_config["unsafe_rain_intensity_max"].is<JsonVariant>() )
		json_config["unsafe_rain_intensity_max"] = DEFAULT_UNSAFE_RAIN_INTENSITY_MAX;

	if ( !json_config["unsafe_rain_intensity_missing"].is<JsonVariant>() )
		json_config["unsafe_rain_intensity_missing"] = DEFAULT_UNSAFE_RAIN_INTENSITY_MISSING;

	if ( !json_config["unsafe_wind_speed_1_active"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_1_active"] = DEFAULT_UNSAFE_WIND_SPEED_1_ACTIVE;

	if ( !json_config["unsafe_wind_speed_2_active"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_2_active"] = DEFAULT_UNSAFE_WIND_SPEED_2_ACTIVE;

	if ( !json_config["unsafe_wind_speed_1_max"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_1_max"] = DEFAULT_UNSAFE_WIND_SPEED_1_MAX;

	if ( !json_config["unsafe_wind_speed_2_max"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_2_max"] = DEFAULT_UNSAFE_WIND_SPEED_2_MAX;

	if ( !json_config["unsafe_wind_speed_1_missing"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_1_missing"] = DEFAULT_UNSAFE_WIND_SPEED_1_MISSING;

	if ( !json_config["unsafe_wind_speed_2_missing"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_2_missing"] = DEFAULT_UNSAFE_WIND_SPEED_2_MISSING;

	if ( !json_config["unsafe_wind_speed_1_delay"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_1_delay"] = DEFAULT_UNSAFE_WIND_SPEED_1_DELAY;

	if ( !json_config["unsafe_wind_speed_2_delay"].is<JsonVariant>() )
		json_config["unsafe_wind_speed_2_delay"] = DEFAULT_UNSAFE_WIND_SPEED_2_DELAY;

}

void AWSConfig::set_missing_lookout_parameters_to_default_values( void )
{
	if ( !json_config["k1"].is<JsonVariant>() )
		json_config["k1"] = DEFAULT_K1;

	if ( !json_config["k2"].is<JsonVariant>() )
		json_config["k2"] = DEFAULT_K3;

	if ( !json_config["k3"].is<JsonVariant>() )
		json_config["k3"] = DEFAULT_K3;

	if ( !json_config["k4"].is<JsonVariant>() )
		json_config["k4"] = DEFAULT_K4;

	if ( !json_config["k5"].is<JsonVariant>() )
		json_config["k5"] = DEFAULT_K5;

	if ( !json_config["k6"].is<JsonVariant>() )
		json_config["k6"] = DEFAULT_K6;

	if ( !json_config["k7"].is<JsonVariant>() )
		json_config["k7"] = DEFAULT_K7;

	if ( !json_config["cc_aag_cloudy"].is<JsonVariant>() )
		json_config["cc_aag_cloudy"] = DEFAULT_CC_AAG_CLOUDY;

	if ( !json_config["cc_aag_overcast"].is<JsonVariant>() )
		json_config["cc_aag_overcast"] = DEFAULT_CC_AAG_OVERCAST;

	if ( !json_config["cc_aws_cloudy"].is<JsonVariant>() )
		json_config["cc_aws_cloudy"] = DEFAULT_CC_AWS_CLOUDY;

	if ( !json_config["cc_aws_overcast"].is<JsonVariant>() )
		json_config["cc_aws_overcast"] = DEFAULT_CC_AWS_OVERCAST;

	if ( !json_config["lookout_enabled"].is<JsonVariant>() )
		json_config["lookout_enabled"] = DEFAULT_LOOKOUT_ENABLED;

	if ( !json_config["cloud_coverage_formula"].is<JsonVariant>() )
		json_config["cloud_coverage_formula"] = DEFAULT_CC_FORMULA_AWS ? 0 : 1;

	set_missing_lookout_safe_parameters_to_default_values();
	set_missing_lookout_unsafe_parameters_to_default_values();
}

void AWSConfig::set_missing_parameters_to_default_values( void )
{
	set_missing_network_parameters_to_default_values();
	set_missing_lookout_parameters_to_default_values();

	if ( !json_config["msas_calibration_offset"].is<JsonVariant>() )
		json_config["msas_calibration_offset"] = DEFAULT_MSAS_CORRECTION;

	if ( !json_config["rain_event_guard_time"].is<JsonVariant>() )
		json_config["rain_event_guard_time"] = DEFAULT_RAIN_EVENT_GUARD_TIME;

	if ( !json_config["tzname"].is<JsonVariant>() )
		json_config["tzname"] = DEFAULT_TZNAME;

	if ( !json_config["automatic_updates"].is<JsonVariant>() )
		json_config["automatic_updates"] = DEFAULT_AUTOMATIC_UPDATES;

	if ( !json_config["data_push"].is<JsonVariant>() )
		json_config["data_push"] = DEFAULT_DATA_PUSH;

	if ( !json_config["push_freq"].is<JsonVariant>() )
		json_config["push_freq"] = DEFAULT_PUSH_FREQ;

	if ( !json_config["discord_enabled"].is<JsonVariant>() )
		json_config["discord_enabled"] = DEFAULT_DISCORD_ENABLED;

	if ( !json_config["discord_wh"].is<JsonVariant>() )
		json_config["discord_wh"] = DEFAULT_DISCORD_WEBHOOK;

	if ( !json_config["ota_url"].is<JsonVariant>() )
		json_config["ota_url"] = DEFAULT_OTA_URL;

}

void AWSConfig::set_parameter( const char *key, const char *val )
{
	etl::string<15>	tmp = val;			// Limit to 15 as it is only used for IP addresses
	json_config[ key ] = tmp.data();
}

void AWSConfig::set_root_ca( JsonVariant &_json_config )
{
	if ( _json_config["root_ca"].is<JsonVariant>() ) {

		if ( strlen( _json_config["root_ca"] ) <= root_ca.capacity() ) {
			root_ca.assign( _json_config["root_ca"].as<const char *>() );
			_json_config.remove( "root_ca" );
			return;
		}
	}
	root_ca.empty();
}

void AWSConfig::update_fs_free_space( void )
{
	fs_free_space = LittleFS.totalBytes() - LittleFS.usedBytes();
}

bool AWSConfig::verify_entries( JsonVariant &proposed_config )
{
	// proposed_config will de facto be modified in this function as
	// config_items is only a way of presenting the items in proposed_config

	JsonObject config_items = proposed_config.as<JsonObject>();
	aws_ip_mode x = aws_ip_mode::dhcp;

	for( JsonPair item : config_items ) {

		switch( str2int( item.key().c_str() )) {	// NOSONAR

			case str2int( "alpaca_iface" ):
			case str2int( "anemometer_model" ):
			case str2int( "cc_aag_cloudy" ):
			case str2int( "cc_aag_overcast" ):
			case str2int( "cc_aws_cloudy" ):
			case str2int( "cc_aws_overcast" ):
			case str2int( "cloud_coverage_formula" ):
			case str2int( "discord_wh" ):
			case str2int( "eth_dns" ):
			case str2int( "eth_gw" ):
			case str2int( "eth_ip" ):
			case str2int( "k1" ):
			case str2int( "k2" ):
			case str2int( "k3" ):
			case str2int( "k4" ):
			case str2int( "k5" ):
			case str2int( "k6" ):
			case str2int( "k7" ):
			case str2int( "ota_url" ):
			case str2int( "pref_iface" ):
			case str2int( "push_freq" ):
			case str2int( "rain_event_guard_time" ):
			case str2int( "remote_server" ):
			case str2int( "root_ca" ):
			case str2int( "safe_cloud_coverage_1_active" ):
			case str2int( "safe_cloud_coverage_2_active" ):
			case str2int( "safe_cloud_coverage_1_delay" ):
			case str2int( "safe_cloud_coverage_2_delay" ):
			case str2int( "safe_cloud_coverage_1_max" ):
			case str2int( "safe_cloud_coverage_2_max" ):
			case str2int( "safe_rain_intensity_active" ):
			case str2int( "safe_rain_intensity_delay" ):
			case str2int( "safe_rain_intensity_max" ):
			case str2int( "safe_wind_speed_active" ):
			case str2int( "safe_wind_speed_delay" ):
			case str2int( "safe_wind_speed_max" ):
			case str2int( "tzname" ):
			case str2int( "unsafe_cloud_coverage_1_active" ):
			case str2int( "unsafe_cloud_coverage_2_active" ):
			case str2int( "unsafe_cloud_coverage_1_delay" ):
			case str2int( "unsafe_cloud_coverage_2_delay" ):
			case str2int( "unsafe_cloud_coverage_1_max" ):
			case str2int( "unsafe_cloud_coverage_2_max" ):
			case str2int( "unsafe_cloud_coverage_1_missing" ):
			case str2int( "unsafe_cloud_coverage_2_missing" ):
			case str2int( "unsafe_rain_event_active" ):
			case str2int( "unsafe_rain_event_missing" ):
			case str2int( "unsafe_rain_intensity_active" ):
			case str2int( "unsafe_rain_intensity_max" ):
			case str2int( "unsafe_rain_intensity_missing" ):
			case str2int( "unsafe_wind_speed_1_active" ):
			case str2int( "unsafe_wind_speed_2_active" ):
			case str2int( "unsafe_wind_speed_1_delay" ):
			case str2int( "unsafe_wind_speed_2_delay" ):
			case str2int( "unsafe_wind_speed_1_max" ):
			case str2int( "unsafe_wind_speed_2_max" ):
			case str2int( "unsafe_wind_speed_1_missing" ):
			case str2int( "unsafe_wind_speed_2_missing" ):
			case str2int( "url_path" ):
			case str2int( "wifi_ap_dns" ):
			case str2int( "wifi_ap_gw" ):
			case str2int( "wifi_ap_ip" ):
			case str2int( "wifi_ap_password" ):
			case str2int( "wifi_ap_ssid" ):
			case str2int( "wifi_mode" ):
			case str2int( "wifi_sta_dns" ):
			case str2int( "wifi_sta_gw" ):
			case str2int( "wifi_sta_ip" ):
			case str2int( "wifi_sta_ip_mode" ):
			case str2int( "wifi_sta_password" ):
			case str2int( "wifi_sta_ssid" ):
			case str2int( "wind_vane_model" ):
				break;
			case str2int( "eth_ip_mode" ):
				x = ( item.value() == "0" ) ? aws_ip_mode::dhcp : aws_ip_mode::fixed;
				break;
			case str2int( "automatic_updates" ):
			case str2int( "data_push" ):
			case str2int( "discord_enabled" ):
			case str2int( "has_bme" ):
			case str2int( "has_dome" ):
			case str2int( "has_gps" ):
			case str2int( "has_mlx" ):
			case str2int( "has_rain_sensor" ):
			case str2int( "has_tsl" ):
			case str2int( "has_ws" ):
			case str2int( "has_wv" ):
			case str2int( "lookout_enabled" ):
				proposed_config[ item.key().c_str() ] = 1;
				break;
			default:
				Serial.printf( "[CONFIGMNGR] [ERROR] Unknown configuration key [%s]\n",  item.key().c_str() );
				return false;
		}
	}

	if ( x == aws_ip_mode::dhcp ) {

		config_items.remove( "eth_ip" );
		config_items.remove( "eth_gw" );
		config_items.remove( "eth_dns" );
	}

	return true;
}
