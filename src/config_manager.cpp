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
#include <ESPAsyncWebSrv.h>
#include <FS.h>
#include <SPIFFS.h>

#include "defaults.h"
#include "common.h"
#include "config_manager.h"
#include "AstroWeatherStation.h"

extern HardwareSerial Serial1;

const unsigned long	MLX_SENSOR			= 0x00000001;
const unsigned long TSL_SENSOR			= 0x00000002;
const unsigned long BME_SENSOR			= 0x00000004;
const unsigned long WIND_VANE_SENSOR	= 0x00000008;
const unsigned long ANEMOMETER_SENSOR	= 0x00000010;
const unsigned long RAIN_SENSOR			= 0x00000020;
const unsigned long	GPS_SENSOR			= 0x00000040;
const unsigned long ALL_SENSORS			= ( MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RAIN_SENSOR | GPS_SENSOR );

const unsigned long	DOME_DEVICE			= 0x00000080;
const unsigned long	ETHERNET_DEVICE		= 0x00000100;
const unsigned long	SC16IS750_DEVICE	= 0x00000200;

extern AstroWeatherStation station;
extern const std::array<std::string, 3> ANEMOMETER_MODEL;

RTC_DATA_ATTR char _can_rollback = 0;	// NOSONAR

//
// Credits to: https://stackoverflow.com/a/16388610
//


bool AWSConfig::can_rollback( void )
{
	return _can_rollback;
}

etl::string_view AWSConfig::get_anemometer_model_str( void )
{
	return etl::string_view( Anemometer::ANEMOMETER_MODEL[ get_parameter<int>( "anemometer_model" ) ].c_str() );	
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

bool AWSConfig::get_has_device( unsigned long dev )
{
	return( devices & dev );
}

etl::string_view AWSConfig::get_json_string_config( void )
{
	static etl::string<5120>	json_string;
	int							i;

	if ( (i= serializeJson( json_config, json_string.data(), json_string.capacity() )) >= json_string.capacity() ) {

		Serial.printf( "[ERROR] Reached configuration string limit (%d > 1024). Please contact support\n", i );
		return etl::string_view( "" );
	}
	return etl::string_view( json_string.data() );
}

etl::string_view AWSConfig::get_wind_vane_model_str( void )
{
	return etl::string_view( Wind_vane::WIND_VANE_MODEL[ get_parameter<int>( "wind_vane_model" ) ].c_str() );
}

void AWSConfig::list_files( void )
{
	File root = SPIFFS.open( "/" );
	File file = root.openNextFile();
 
	while( file ) {
 
      Serial.printf( "[DEBUG] Filename: %05d /%s\n", file.size(), file.name() );
      file = root.openNextFile();
	}	
	close( root );
}

bool AWSConfig::load( bool _debug_mode  )
{
	debug_mode = _debug_mode;
	
	if ( !SPIFFS.begin( true )) {

		Serial.printf( "[ERROR] Could not access flash filesystem, bailing out!\n" );
		return false;
	}
	if ( debug_mode )
		list_files();

	return read_config();
}

bool AWSConfig::read_config( void )
{
	read_hw_info_from_nvs();

	read_root_ca();
	
	if ( !read_file( "/aws.conf"  ) ) {
		
		if ( !read_file( "/aws.conf.dfl" )) {
			
			Serial.printf( "\n[ERROR] Could not read config file.\n" );
			return false;
		}
		Serial.printf( "[INFO] Using minimal/factory config file.\n" );
	}

	devices |= DOME_DEVICE * ( json_config.containsKey( "has_dome" ) ? json_config["has_dome"].as<int>() : DEFAULT_HAS_DOME );
	devices |= BME_SENSOR * ( json_config.containsKey( "has_bme" ) ? json_config["has_bme"].as<int>() : DEFAULT_HAS_BME );
	devices |= GPS_SENSOR * ( json_config.containsKey( "has_gps" ) ? json_config["has_gps"].as<int>() : DEFAULT_HAS_GPS );
	devices |= MLX_SENSOR * ( json_config.containsKey( "has_mlx" ) ? json_config["has_mlx"].as<int>() : DEFAULT_HAS_MLX );
	devices |= RAIN_SENSOR * ( json_config.containsKey( "has_rain_sensor" ) ? json_config["has_rain_sensor"].as<int>() : DEFAULT_HAS_RAIN_SENSOR );
	devices |= TSL_SENSOR * ( json_config.containsKey( "has_tsl" ) ? json_config["has_tsl"].as<int>() : DEFAULT_HAS_TSL );
	devices |= ANEMOMETER_SENSOR * ( json_config.containsKey( "has_ws" ) ? json_config["has_ws"].as<int>() : DEFAULT_HAS_WS );
	devices |= WIND_VANE_SENSOR * ( json_config.containsKey( "has_wv" ) ? json_config["has_wv"].as<int>() : DEFAULT_HAS_WV );
	
	set_missing_parameters_to_default_values();

	return true;
}

void AWSConfig::read_root_ca( void )
{
	File 	file = SPIFFS.open( "/root_ca.txt", FILE_READ );
	int		s;

	if ( !file ) {

		Serial.printf( "[ERROR] Cannot read ROOT CA file. Using default CA.\n");
		s = strlen( DEFAULT_ROOT_CA );
		if ( s > root_ca.capacity() )
			Serial.printf( "[BUG] Size of default ROOT CA is too big [%d > %d].\n", s, root_ca.capacity() );
		else
			root_ca.assign( DEFAULT_ROOT_CA );
		return;
	}

	if (!( s = file.size() )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Empty ROOT CA file. Using default CA.\n" );
		s = strlen( DEFAULT_ROOT_CA );
		if ( s > root_ca.capacity() )
			Serial.printf( "[BUG] Size of default ROOT CA is too big [%d > %d].\n", s, root_ca.capacity() );
		else
			root_ca.assign( DEFAULT_ROOT_CA );
		return;
	}

	if ( s > root_ca.capacity() ) {

		Serial.printf( "[ERROR] ROOT CA file is too big. Using default CA.\n" );
		s = strlen( DEFAULT_ROOT_CA );
		if ( s > root_ca.capacity() )
			Serial.printf( "[BUG] Size of default ROOT CA is too big [%d > %d].\n", s, root_ca.capacity() );
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
	etl::string<5120>	json_string;
	int					s;

	json_string.assign( 5120, '\0' );
	// flawfinder: ignore
	File file = SPIFFS.open( filename, FILE_READ );

	if ( !file ) {

		Serial.printf( "[ERROR] Cannot read config file [%s]\n", filename );
		return false;
	}

	if (!( s = file.size() )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Empty config file [%s]\n", filename );
		return false;
	}

	if ( s > json_string.capacity() ) {

			Serial.printf( "[ERROR] Configuration file is too big and will be truncated. Trying to rollback configuration. Please contact support!\n" );
			if ( can_rollback() && rollback() ) {
					
					station.send_alarm( "Configuration error", "Configuration file is too big. Rolled back configuration and rebooting." );
					station.reboot();

			}

			station.send_alarm( "Configuration error", "Configuration file is too big. Could not roll back configuration, falling back to default configuration." );
			return false;
	}

	file.readBytes( json_string.data() , s );
	file.close();

	if ( debug_mode )
		Serial.printf( "[DEBUG] File successfuly read... ");

	if ( DeserializationError::Ok == deserializeJson( json_config, json_string.data() )) {

		if ( debug_mode )
			Serial.printf( "and configuration format is valid... OK!\n");

		return true;
	}

	if ( debug_mode )
		Serial.printf( "but configuration has been corrupted...\n");

	return false;
}

bool AWSConfig::read_hw_info_from_nvs( void )
{
	Preferences nvs;
	char	x;
	
	Serial.printf( "[INFO] Reading NVS values.\n" );
	nvs.begin( "aws", false );
	if ( !nvs.getString( "pcb_version", pcb_version.data(), pcb_version.capacity() )) {

		Serial.printf( "[PANIC] Could not get PCB version from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	if ( static_cast<byte>(( pwr_mode = (aws_pwr_src) nvs.getChar( "pwr_mode", 127 ))) == 127 ) {

		Serial.printf( "[PANIC] Could not get PowerMode from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	if ( ( x = nvs.getChar( "has_sc16is750", 127 )) == 127 ) {

		Serial.printf( "[PANIC] Could not get SC16IS750 presence from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	devices |= ( x == 0 ) ? 0 : SC16IS750_DEVICE;

	if ( ( x = nvs.getChar( "has_ethernet", 127 )) == 127 ) {

		printf( "[PANIC] Could not get PowerMode from NVS. Please contact support.\n" );
		nvs.end();
		return false;
	}
	devices |= ( x == 0 ) ? false : ETHERNET_DEVICE;
	nvs.end();
	return true;
}

bool AWSConfig::rollback()
{
	if ( !_can_rollback ) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] No configuration to rollback.\n");
		return true;

	}

	Serial.printf( "[INFO] Rolling back last submitted configuration.\n" );

	if ( !SPIFFS.begin( true )) {

		Serial.printf( "[ERROR] Could not open filesystem.\n" );
		return false;
	}

	SPIFFS.remove( "/aws.conf" );
	SPIFFS.rename( "/aws.conf.bak", "/aws.conf" );
	Serial.printf( "[INFO] Rollback successful.\n" );
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
	
	Serial.printf( "[INFO] Saving submitted configuration.\n" );

	if ( !SPIFFS.begin( true )) {

		Serial.printf( "[ERROR] Could not open filesystem.\n" );
		return false;
	}
	
	SPIFFS.remove( "/aws.conf.bak.try" );
	SPIFFS.rename( "/aws.conf", "/aws.conf.bak.try" );

	SPIFFS.remove( "/aws.conf.try" );
	// flawfinder: ignore
	File file = SPIFFS.open( "/aws.conf.try", FILE_WRITE );
	s = serializeJson( _json_config, file );
	file.close();
	if ( !s ) {

		SPIFFS.remove( "/aws.conf" );
		SPIFFS.remove( "/aws.conf.try" );
		SPIFFS.rename( "/aws.conf.bak.try", "/aws.conf" );
		Serial.printf( "[ERROR] Empty configuration file, rolling back.\n" );
		return false;
		
	}
	if ( s > MAX_CONFIG_FILE_SIZE ) {

		SPIFFS.remove( "/aws.conf.try" );
		SPIFFS.rename( "/aws.conf.bak.try", "/aws.conf" );
		Serial.printf( "[ERROR] Configuration file is too big [%d bytes].\n" );
		station.send_alarm( "Configuration error", "Configuration file is too big. Not applying changes!" );
		return false;

	}
	if ( debug_mode )
		list_files();
	SPIFFS.remove( "/aws.conf" );
Serial.printf(" REMOVED AWS.CONF\n");
	if ( debug_mode )
		list_files();
	SPIFFS.rename( "/aws.conf.try", "/aws.conf" );
	SPIFFS.rename( "/aws.conf.bak.try", "/aws.conf.bak" );
	Serial.printf( "[INFO] Wrote %d bytes, configuration save successful.\n", s );

	SPIFFS.remove( "/root_ca.txt.try" );
	SPIFFS.rename( "/root_ca.txt", "/root_ca.txt.try" );

	file = SPIFFS.open( "/root_ca.txt.try", FILE_WRITE );
	s = file.print( root_ca.data() );
	file.close();
	Serial.printf( "[INFO] Wrote %d bytes of ROOT CA.\n", s );
	SPIFFS.rename( "/root_ca.txt.try", "/root_ca.txt" );
	
	_can_rollback = 1;
	if ( debug_mode )
		list_files();

	return true;
}

void AWSConfig::set_missing_network_parameters_to_default_values( void )
{
	if ( !json_config.containsKey( "wifi_ap_ssid" ))
		json_config["wifi_ap_ssid"] = DEFAULT_WIFI_AP_SSID;

	if ( !json_config.containsKey( "config_port" ))
		json_config["config_port"] = DEFAULT_CONFIG_PORT;
		
	if ( !json_config.containsKey( "eth_ip" ))
		json_config["eth_ip"] = DEFAULT_ETH_IP;

	if ( !json_config.containsKey( "eth_dns" ))
		json_config["eth_dns"] = DEFAULT_ETH_DNS;

	if ( !json_config.containsKey( "eth_ip_mode" ))
		json_config["eth_ip_mode"] = static_cast<int>( DEFAULT_ETH_IP_MODE );

	if ( !json_config.containsKey( "pref_iface" ))
		json_config["pref_iface"] = static_cast<int>( aws_iface::wifi_ap );

	if ( !json_config.containsKey( "remote_server" ))
		json_config["remote_server"] = DEFAULT_SERVER;

	if ( !json_config.containsKey( "wifi_sta_ssid" ))
		json_config["wifi_sta_ssid"] = DEFAULT_WIFI_STA_SSID;

	if ( !json_config.containsKey( "url_path" ))
		json_config["url_path"] = DEFAULT_URL_PATH;

	if ( !json_config.containsKey( "wifi_ap_dns" ))
		json_config["wifi_ap_dns"] = DEFAULT_WIFI_AP_DNS;

	if ( !json_config.containsKey( "wifi_ap_gw" ))
		json_config["wifi_ap_gw"] = DEFAULT_WIFI_AP_GW;

	if ( !json_config.containsKey( "wifi_ap_ip" ))
		json_config["wifi_ap_ip"] = DEFAULT_WIFI_AP_IP;

	if ( !json_config.containsKey( "wifi_ap_password" ))
		json_config["wifi_ap_password"] = DEFAULT_WIFI_AP_PASSWORD;

	if ( !json_config.containsKey( "wifi_mode" ))
		json_config["wifi_mode"] = static_cast<int>( DEFAULT_WIFI_MODE );

	if ( !json_config.containsKey( "wifi_sta_dns" ))
		json_config["wifi_sta_dns"] = DEFAULT_WIFI_STA_DNS;

	if ( !json_config.containsKey( "wifi_sta_gw" ))
		json_config["wifi_sta_gw"] = DEFAULT_WIFI_STA_GW;

	if ( !json_config.containsKey( "wifi_sta_ip" ))
		json_config["wifi_sta_ip"] = DEFAULT_WIFI_STA_IP;

	if ( !json_config.containsKey( "wifi_sta_ip_mode" ))
		json_config["wifi_sta_ip_mode"] = static_cast<int>( DEFAULT_WIFI_STA_IP_MODE );

	if ( !json_config.containsKey( "wifi_sta_password" ))
		json_config["wifi_sta_password"] = DEFAULT_WIFI_STA_PASSWORD;
}

void AWSConfig::set_missing_lookout_safe_parameters_to_default_values( void )
{
	if ( !json_config.containsKey( "safe_cloud_coverage_1_active" ))
		json_config["safe_cloud_coverage_1_active"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_ACTIVE;

	if ( !json_config.containsKey( "safe_cloud_coverage_2_active" ))
		json_config["safe_cloud_coverage_2_active"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_ACTIVE;

	if ( !json_config.containsKey( "safe_cloud_coverage_1_delay" ))
		json_config["safe_cloud_coverage_1_delay"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_DELAY;

	if ( !json_config.containsKey( "safe_cloud_coverage_2_delay" ))
		json_config["safe_cloud_coverage_2_delay"] = DEFAULT_SAFE_CLOUD_COVERAGE_2_DELAY;

	if ( !json_config.containsKey( "safe_cloud_coverage_1_max" ))
		json_config["safe_cloud_coverage_1_max"] = DEFAULT_SAFE_CLOUD_COVERAGE_1_MAX;

	if ( !json_config.containsKey( "safe_cloud_coverage_2_max" ))
		json_config["safe_cloud_coverage_2_max"] = DEFAULT_SAFE_CLOUD_COVERAGE_2_MAX;

	if ( !json_config.containsKey( "safe_rain_intensity_active" ))
		json_config["safe_rain_intensity_active"] = DEFAULT_SAFE_RAIN_INTENSITY_ACTIVE;

	if ( !json_config.containsKey( "safe_rain_intensity_delay" ))
		json_config["safe_rain_intensity_delay"] = DEFAULT_SAFE_RAIN_INTENSITY_DELAY;

	if ( !json_config.containsKey( "safe_rain_intensity_max" ))
		json_config["safe_rain_intensity_max"] = DEFAULT_SAFE_RAIN_INTENSITY_MAX;

	if ( !json_config.containsKey( "safe_wind_speed_active" ))
		json_config["safe_wind_speed_1_active"] = DEFAULT_SAFE_WIND_SPEED_1_ACTIVE;

	if ( !json_config.containsKey( "safe_wind_speed_max" ))
		json_config["safe_wind_speed_1_max"] = DEFAULT_SAFE_WIND_SPEED_1_MAX;

	if ( !json_config.containsKey( "safe_wind_speed_delay" ))
		json_config["safe_wind_speed_1_delay"] = DEFAULT_SAFE_WIND_SPEED_1_DELAY;
}

void AWSConfig::set_missing_lookout_unsafe_parameters_to_default_values( void )
{
	if ( !json_config.containsKey( "unsafe_cloud_coverage_1_active" ))
		json_config["unsafe_cloud_coverage_1_active"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_ACTIVE;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_2_active" ))
		json_config["unsafe_cloud_coverage_2_active"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_ACTIVE;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_1_delay" ))
		json_config["unsafe_cloud_coverage_1_delay"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_DELAY;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_2_delay" ))
		json_config["unsafe_cloud_coverage_2_delay"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_DELAY;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_1_max" ))
		json_config["unsafe_cloud_coverage_1_max"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_MAX;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_2_max" ))
		json_config["unsafe_cloud_coverage_2_max"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_MAX;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_1_missing" ))
		json_config["unsafe_cloud_coverage_1_missing"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_1_MISSING;

	if ( !json_config.containsKey( "unsafe_cloud_coverage_2_missing" ))
		json_config["unsafe_cloud_coverage_2_missing"] = DEFAULT_UNSAFE_CLOUD_COVERAGE_2_MISSING;

	if ( !json_config.containsKey( "unsafe_rain_event_active" ))
		json_config["unsafe_rain_event_active"] = DEFAULT_UNSAFE_RAIN_EVENT_ACTIVE;

	if ( !json_config.containsKey( "unsafe_rain_event_missing" ))
		json_config["unsafe_rain_event_missing"] = DEFAULT_UNSAFE_RAIN_EVENT_MISSING;

	if ( !json_config.containsKey( "unsafe_rain_intensity_active" ))
		json_config["unsafe_rain_intensity_active"] = DEFAULT_UNSAFE_RAIN_INTENSITY_ACTIVE;

	if ( !json_config.containsKey( "unsafe_rain_intensity_max" ))
		json_config["unsafe_rain_intensity_max"] = DEFAULT_UNSAFE_RAIN_INTENSITY_MAX;

	if ( !json_config.containsKey( "unsafe_rain_intensity_missing" ))
		json_config["unsafe_rain_intensity_missing"] = DEFAULT_UNSAFE_RAIN_INTENSITY_MISSING;

	if ( !json_config.containsKey( "unsafe_wind_speed_1_active" ))
		json_config["unsafe_wind_speed_1_active"] = DEFAULT_UNSAFE_WIND_SPEED_1_ACTIVE;

	if ( !json_config.containsKey( "unsafe_wind_speed_2_active" ))
		json_config["unsafe_wind_speed_2_active"] = DEFAULT_UNSAFE_WIND_SPEED_2_ACTIVE;

	if ( !json_config.containsKey( "unsafe_wind_speed_1_max" ))
		json_config["unsafe_wind_speed_1_max"] = DEFAULT_UNSAFE_WIND_SPEED_1_MAX;

	if ( !json_config.containsKey( "unsafe_wind_speed_2_max" ))
		json_config["unsafe_wind_speed_2_max"] = DEFAULT_UNSAFE_WIND_SPEED_2_MAX;

	if ( !json_config.containsKey( "unsafe_wind_speed_1_missing" ))
		json_config["unsafe_wind_speed_1_missing"] = DEFAULT_UNSAFE_WIND_SPEED_1_MISSING;

	if ( !json_config.containsKey( "unsafe_wind_speed_2_missing" ))
		json_config["unsafe_wind_speed_2_missing"] = DEFAULT_UNSAFE_WIND_SPEED_2_MISSING;

	if ( !json_config.containsKey( "unsafe_wind_speed_1_delay" ))
		json_config["unsafe_wind_speed_1_delay"] = DEFAULT_UNSAFE_WIND_SPEED_1_DELAY;

	if ( !json_config.containsKey( "unsafe_wind_speed_2_delay" ))
		json_config["unsafe_wind_speed_2_delay"] = DEFAULT_UNSAFE_WIND_SPEED_2_DELAY;

}
void AWSConfig::set_missing_lookout_parameters_to_default_values( void )
{
	if ( !json_config.containsKey( "k1" ))
		json_config["k1"] = DEFAULT_K1;

	if ( !json_config.containsKey( "k2" ))
		json_config["k2"] = DEFAULT_K3;

	if ( !json_config.containsKey( "k3" ))
		json_config["k3"] = DEFAULT_K3;

	if ( !json_config.containsKey( "k4" ))
		json_config["k4"] = DEFAULT_K4;

	if ( !json_config.containsKey( "k5" ))
		json_config["k5"] = DEFAULT_K5;

	if ( !json_config.containsKey( "k6" ))
		json_config["k6"] = DEFAULT_K6;

	if ( !json_config.containsKey( "k7" ))
		json_config["k7"] = DEFAULT_K7;

	if ( !json_config.containsKey( "lookout_enabled" ))
		json_config["lookout_enabled"] = DEFAULT_LOOKOUT_ENABLED;

	set_missing_lookout_safe_parameters_to_default_values();
	set_missing_lookout_unsafe_parameters_to_default_values();
}

void AWSConfig::set_missing_parameters_to_default_values( void )
{
	set_missing_network_parameters_to_default_values();
	set_missing_lookout_parameters_to_default_values();
		
	if ( !json_config.containsKey( "msas_calibration_offset" ))
		json_config["msas_calibration_offset"] = DEFAULT_MSAS_CORRECTION;

	if ( !json_config.containsKey( "rain_event_guard_time" ))
		json_config["rain_event_guard_time"] = DEFAULT_RAIN_EVENT_GUARD_TIME;

	if ( !json_config.containsKey( "tzname" ))
		json_config["tzname"] = DEFAULT_TZNAME;
}

void AWSConfig::set_root_ca( JsonVariant &_json_config )
{
	if ( _json_config.containsKey( "root_ca" )) {

		if ( strlen( _json_config["root_ca"] ) <= root_ca.capacity() ) {
			root_ca.assign( _json_config["root_ca"].as<const char *>() );
			_json_config.remove( "root_ca" );
			return;
		}
	}
	root_ca.empty();
}

bool AWSConfig::verify_entries( JsonVariant &proposed_config )
{
	// proposed_config will de facto be modified in this function as
	// config_items is only a way of presenting the items in proposed_config

	JsonObject config_items = proposed_config.as<JsonObject>();
	aws_ip_mode x = aws_ip_mode::dhcp;
	
	for( JsonPair item : config_items ) {

		switch( str2int( item.key().c_str() )) {

			case str2int( "alpaca_iface" ):
			case str2int( "anemometer_model" ):
			case str2int( "cloud_coverage_formula" ):
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
			case str2int( "pref_iface" ):
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
				Serial.printf( "[ERROR] Unknown configuration key [%s]\n",  item.key().c_str() );
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
