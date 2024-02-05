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
extern const std::array<std::string, 3> _anemometer_model;

RTC_DATA_ATTR char _can_rollback = 0;

//
// Credits to: https://stackoverflow.com/a/16388610
//
constexpr unsigned int str2int( const char* str, int h = 0 )
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

AWSConfig::AWSConfig( void )
{
	pcb_version[ 0 ] = 0;
}

bool AWSConfig::can_rollback( void )
{
	return _can_rollback;
}

aws_iface AWSConfig::get_alpaca_iface( void )
{
	return network_config.get_alpaca_iface();
}

uint8_t AWSConfig::get_anemometer_model( void )
{
	return anemometer_model;
}

const char *AWSConfig::get_anemometer_model_str( void )
{
	return Anemometer::_anemometer_model[ anemometer_model ].c_str();
}

bool AWSConfig::get_close_dome_on_rain( void )
{
	return close_dome_on_rain;
}

aws_iface AWSConfig::get_config_iface( void )
{
	return network_config.get_config_iface();
}

uint16_t AWSConfig::get_config_port( void )
{
	return config_port;
}

char *AWSConfig::get_eth_dns( void )
{
	return network_config.get_eth_dns();
}

char *AWSConfig::get_eth_gw( void )
{
	return network_config.get_eth_gw();
}

char *AWSConfig::get_eth_ip( void )
{
	return network_config.get_eth_ip();
}

aws_ip_mode AWSConfig::get_eth_ip_mode( void )
{
	return network_config.get_eth_ip_mode();
}

bool AWSConfig::get_has_bme( void )
{
	return ( devices & BME_SENSOR );
}

bool AWSConfig::get_has_dome( void )
{
	return ( devices & DOME_DEVICE );
}

bool AWSConfig::get_has_ethernet( void )
{
	return ( devices & ETHERNET_DEVICE );
}

bool AWSConfig::get_has_gps( void )
{
	return ( devices & GPS_SENSOR );
}

bool AWSConfig::get_has_mlx( void )
{
	return ( devices & MLX_SENSOR );
}

bool AWSConfig::get_has_rain_sensor( void )
{
	return ( devices & RAIN_SENSOR );
}

bool AWSConfig::get_has_sc16is750( void )
{
	return ( devices & SC16IS750_DEVICE );
}

bool AWSConfig::get_has_tsl( void )
{
	return ( devices & TSL_SENSOR );
}

bool AWSConfig::get_has_ws( void )
{
	return ( devices & ANEMOMETER_SENSOR );
}

bool AWSConfig::get_has_wv( void )
{
	return ( devices & WIND_VANE_SENSOR );
}

char *AWSConfig::get_json_string_config( void )
{
	DynamicJsonDocument	aws_json_config(1024);
	// FIXME: unless free()'d by caller this is a memory leak
	char *json_string = static_cast<char *>( malloc( 1024 ) );
	// flawfinder: ignore
	char buf[64];

	aws_json_config["tzname"] = tzname;
	aws_json_config["pref_iface"] = static_cast<byte>( network_config.get_pref_iface() );
    aws_json_config["eth_ip_mode"] = static_cast<byte>( network_config.get_eth_ip_mode() );
	if ( network_config.get_eth_ip_mode() == aws_ip_mode::dhcp ) {

		snprintf( buf, 64, "%s/%d", station.get_eth_ip()->toString().c_str(), station.get_eth_cidr_prefix() );
		aws_json_config["eth_ip"] = buf;
		snprintf( buf, 64, "%s",  station.get_eth_gw()->toString().c_str() );
		aws_json_config["eth_gw"] = buf;
		snprintf( buf, 64, "%s",  station.get_eth_dns()->toString().c_str() );
		aws_json_config["eth_dns"] = buf;

	} else {

		aws_json_config["eth_ip"] = network_config.get_eth_ip();
		aws_json_config["eth_gw"] = network_config.get_eth_gw();
		aws_json_config["eth_dns"] = network_config.get_eth_dns();

	}
	aws_json_config["sta_ssid"] = network_config.get_wifi_sta_ssid();
	aws_json_config["wifi_sta_password"] = network_config.get_wifi_sta_password();
	aws_json_config["wifi_sta_ip_mode"] = static_cast<byte>( network_config.get_wifi_sta_ip_mode() );

	if ( network_config.get_wifi_sta_ip_mode() == aws_ip_mode::dhcp ) {

		snprintf( buf, 64, "%s/%d", station.get_wifi_sta_ip()->toString().c_str(), station.get_wifi_sta_cidr_prefix() );
		aws_json_config["wifi_sta_ip"] = buf;
		snprintf( buf, 64, "%s",  station.get_wifi_sta_gw()->toString().c_str() );
		aws_json_config["wifi_sta_gw"] = buf;
		snprintf( buf, 64, "%s",  station.get_wifi_sta_dns()->toString().c_str() );
		aws_json_config["wifi_sta_dns"] = buf;

	} else {

		aws_json_config["wifi_sta_ip"] = network_config.get_wifi_sta_ip();
		aws_json_config["wifi_sta_gw"] = network_config.get_eth_gw();
		aws_json_config["wifi_sta_dns"] = network_config.get_eth_dns();

	}

	aws_json_config["ap_ssid"] = network_config.get_wifi_ap_ssid();
	aws_json_config["wifi_ap_password"] = network_config.get_wifi_ap_password();
	aws_json_config["wifi_ap_ip"] = network_config.get_wifi_ap_ip();
	aws_json_config["wifi_ap_gw"] = network_config.get_wifi_ap_gw();

	aws_json_config["remote_server"] = remote_server;
	aws_json_config["url_path"] = url_path;
	aws_json_config["has_bme"] = get_has_bme();
	aws_json_config["has_tsl"] = get_has_tsl();
	aws_json_config["has_mlx"] = get_has_mlx();
	aws_json_config["has_ws"] = get_has_ws();
	aws_json_config["anemometer_model"] = anemometer_model;
	aws_json_config["has_wv"] = get_has_wv();
	aws_json_config["windvane_model"] = wind_vane_model;
	aws_json_config["has_rain_sensor"] = get_has_rain_sensor();
	aws_json_config["rain_event_guard_time"] = rain_event_guard_time;
	aws_json_config["has_gps"] = get_has_gps();
	aws_json_config["wifi_mode"] = static_cast<byte>( network_config.get_wifi_mode() );
	aws_json_config["alpaca_iface"] = static_cast<byte>( network_config.get_alpaca_iface() );
	aws_json_config["config_iface"] = static_cast<byte>( network_config.get_config_iface() );

	aws_json_config["has_sc16is750" ] = get_has_sc16is750();
	
	aws_json_config["has_dome"] = get_has_dome();
	aws_json_config["close_dome_on_rain"] = close_dome_on_rain;
	
	aws_json_config["has_ethernet"] = get_has_ethernet();
	
	if ( serializeJson( aws_json_config, json_string, 1024 ) >= 1024 ) {

		Serial.printf( "[ERROR] Reached configuration string limit. Please contact support\n" );
		free( json_string );
		return nullptr;
	}
	return json_string;
}

float AWSConfig::get_msas_calibration_offset( void )
{
	return msas_calibration_offset;
}

char *AWSConfig::get_pcb_version( void )
{
	return pcb_version;
}

aws_iface AWSConfig::get_pref_iface( void )
{
	return network_config.get_pref_iface();
}

aws_pwr_src AWSConfig::get_pwr_mode( void )
{
	return pwr_mode;
}

uint16_t AWSConfig::get_rain_event_guard_time( void )
{
	return rain_event_guard_time;
}

char *AWSConfig::get_remote_server( void )
{
	return remote_server;
}

char *AWSConfig::get_root_ca( void )
{
	return network_config.get_root_ca();
}

char *AWSConfig::get_tzname( void )
{
	return tzname;
}

char *AWSConfig::get_url_path( void )
{
	return url_path;
}

char *AWSConfig::get_wifi_ap_dns( void )
{
	return network_config.get_wifi_ap_dns();
}

char *AWSConfig::get_wifi_ap_gw( void )
{
	return network_config.get_wifi_ap_gw();
}

char *AWSConfig::get_wifi_ap_ip( void )
{
	return network_config.get_wifi_ap_ip();
}

char *AWSConfig::get_wifi_ap_ssid( void )
{
	return network_config.get_wifi_ap_ssid();
}

aws_wifi_mode AWSConfig::get_wifi_mode( void )
{
	return network_config.get_wifi_mode();
}

char *AWSConfig::get_wifi_ap_password( void )
{
	return network_config.get_wifi_ap_password();
}

char *AWSConfig::get_wifi_sta_dns( void )
{
	return network_config.get_wifi_sta_dns();
}

char *AWSConfig::get_wifi_sta_gw( void )
{
	return network_config.get_wifi_sta_gw();
}

char *AWSConfig::get_wifi_sta_ip( void )
{
	return network_config.get_wifi_sta_ip();
}

aws_ip_mode AWSConfig::get_wifi_sta_ip_mode( void )
{
	return network_config.get_wifi_sta_ip_mode();
}

char *AWSConfig::get_wifi_sta_password( void )
{
	return network_config.get_wifi_sta_password();
}

char *AWSConfig::get_wifi_sta_ssid( void )
{
	return network_config.get_wifi_sta_ssid();
}

uint8_t AWSConfig::get_wind_vane_model( void )
{
	return wind_vane_model;
}

const char *AWSConfig::get_wind_vane_model_str( void )
{
	return Wind_vane::_windvane_model[ wind_vane_model ].c_str();
}

bool AWSConfig::load( bool _debug_mode  )
{
	debug_mode = _debug_mode;
	
	if ( !SPIFFS.begin( true )) {

		Serial.printf( "[ERROR] Could not access flash filesystem, bailing out!\n" );
		return false;
	}
	return read_config();
}

bool AWSConfig::read_config( void )
{
	DynamicJsonDocument	aws_json_config(768);

	read_hw_info_from_nvs();

	if ( !read_file( "/aws.conf", aws_json_config ) ) {
		
		if ( !read_file( "/aws.conf.dfl", aws_json_config )) {
			
			Serial.printf( "\n[ERROR] Could not read config file.\n" );
			return false;
		}
		Serial.printf( "[INFO] Using minimal/factory config file.\n" );
	}

	network_config.commit_config( aws_json_config );
	
	wind_vane_model = aws_json_config.containsKey( "windvane_model" ) ? aws_json_config["windvane_model"] : 0;
	anemometer_model = aws_json_config.containsKey( "anemometer_model" ) ? aws_json_config["anemometer_model"] : 0;
	config_port = aws_json_config.containsKey( "config_port" ) ? aws_json_config["config_port"] : DEFAULT_CONFIG_PORT;

	set_parameter( aws_json_config, "remote_server", &remote_server, DEFAULT_SERVER );
	set_parameter( aws_json_config, "url_path", &url_path, DEFAULT_URL_PATH );
	set_parameter( aws_json_config, "tzname", &tzname, DEFAULT_TZNAME );

	msas_calibration_offset = aws_json_config.containsKey( "msas_calibration_offset" ) ? atof( aws_json_config["msas_calibration_offset"] ) : DEFAULT_MSAS_CORRECTION;
	close_dome_on_rain = aws_json_config.containsKey( "close_dome_on_rain" ) ? ( aws_json_config["close_dome_on_rain"] == 1 ) : DEFAULT_CLOSE_DOME_ON_RAIN;
	devices |= DOME_DEVICE * ( aws_json_config.containsKey( "has_dome" ) ? aws_json_config["has_dome"] : DEFAULT_HAS_DOME );
	devices |= BME_SENSOR * ( aws_json_config.containsKey( "has_bme" ) ? aws_json_config["has_bme"] : DEFAULT_HAS_BME );
	devices |= GPS_SENSOR * ( aws_json_config.containsKey( "has_gps" ) ? aws_json_config["has_gps"] : DEFAULT_HAS_GPS );
	devices |= MLX_SENSOR * ( aws_json_config.containsKey( "has_mlx" ) ? aws_json_config["has_mlx"] : DEFAULT_HAS_MLX );
	devices |= RAIN_SENSOR * ( aws_json_config.containsKey( "has_rain_sensor" ) ? aws_json_config["has_rain_sensor"] : DEFAULT_HAS_RAIN_SENSOR );
	rain_event_guard_time = aws_json_config.containsKey( "rain_event_guard_time" ) ? atof( aws_json_config["rain_event_guard_time"] ) : DEFAULT_RAIN_EVENT_GUARD_TIME;
	devices |= TSL_SENSOR * ( aws_json_config.containsKey( "has_tsl" ) ? aws_json_config["has_tsl"] : DEFAULT_HAS_TSL );
	devices |= ANEMOMETER_SENSOR * ( aws_json_config.containsKey( "has_ws" ) ? aws_json_config["has_ws"] : DEFAULT_HAS_WS );
	devices |= WIND_VANE_SENSOR * ( aws_json_config.containsKey( "has_wv" ) ? aws_json_config["has_wv"] : DEFAULT_HAS_WV );

	return true;
}

bool AWSConfig::read_file( const char *filename, JsonDocument &_json_config )
{
	char	*json_string;
	int		s = 0;

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

	json_string = static_cast<char *>( malloc( s ) );
	file.readBytes( json_string, s);
	file.close();

	if ( debug_mode )
		Serial.printf( "[DEBUG] File successfuly read... ");

	if ( DeserializationError::Ok == deserializeJson( _json_config, json_string )) {

		if ( debug_mode )
			Serial.printf( "and configuration format is valid... OK!\n");

		free( json_string );
		return true;
	}

	if ( debug_mode )
		Serial.printf( "but configuration has been corrupted...\n");

	free( json_string );
	return false;
}

bool AWSConfig::read_hw_info_from_nvs( void )
{
	Preferences nvs;
	char	x;
	
	Serial.printf( "[INFO] Reading NVS values.\n" );
	nvs.begin( "aws", false );
	if ( !nvs.getString( "pcb_version", pcb_version, 8 )) {

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
	uint8_t	buf[ 4096 ];

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
	// flawfinder: ignore
	File file = SPIFFS.open( "/aws.conf", FILE_WRITE );
	// flawfinder: ignore
	File filebak = SPIFFS.open( "/aws.conf.bak" );

	while( filebak.available() ) {

		size_t	i = filebak.readBytes( reinterpret_cast<char *>( buf ), 4096 );
		file.write( buf, i );
	}

	file.close();
	filebak.close();
	SPIFFS.remove( "/aws.conf.bak" );
	Serial.printf( "[INFO] Rollback successful.\n" );
	_can_rollback = 0;
	return true;
}

bool AWSConfig::save_runtime_configuration( JsonVariant &json_config )
{
	uint8_t	buf[ 4096 ];

	if ( !verify_entries( json_config ))
		return false;

	Serial.printf( "[INFO] Saving submitted configuration.\n" );

	if ( !SPIFFS.begin( true )) {

		Serial.printf( "[ERROR] Could not open filesystem.\n" );
		return false;
	}
	// flawfinder: ignore
	File file = SPIFFS.open( "/aws.conf" );
	// flawfinder: ignore
	File filebak = SPIFFS.open( "/aws.conf.bak", FILE_WRITE );

	while( file.available() ) {

		size_t	i = file.readBytes( reinterpret_cast<char *>( buf ), 4096 );
		filebak.write( buf, i );
	}
	file.close();
	filebak.close();

	// flawfinder: ignore
	file = SPIFFS.open( "/aws.conf", FILE_WRITE );
	serializeJson( json_config, file );
	file.close();
	Serial.printf( "[INFO] Save successful.\n" );
	_can_rollback = 1;
	return true;
}

bool AWSConfig::set_parameter( JsonDocument &aws_json_config, const char *config_key, char **config_item, const char *default_value )
{
	if ( aws_json_config.containsKey( config_key ) ) {

		if ( *config_item ) {

			if ( strcmp( *config_item, aws_json_config[config_key] )) {

				if ( *config_item != default_value )
					free( *config_item );
				*config_item = strdup( aws_json_config[config_key] );
				return true;
			}

		} else {

			*config_item = strdup( aws_json_config[config_key] );
			return true;
		}

	} else {

		if ( *config_item && ( *config_item != default_value ))
			free( *config_item );
		*config_item = const_cast<char *>( default_value );
		return true;
	}
	return false;
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
			case str2int( "ap_ssid" ):
			case str2int( "eth_dns" ):
			case str2int( "eth_gw" ):
			case str2int( "eth_ip" ):
			case str2int( "pref_iface" ):
			case str2int( "rain_event_guard_time" ):
			case str2int( "remote_server" ):
			case str2int( "root_ca" ):
			case str2int( "sta_ssid" ):
			case str2int( "tzname" ):
			case str2int( "url_path" ):
			case str2int( "wifi_ap_dns" ):
			case str2int( "wifi_ap_gw" ):
			case str2int( "wifi_ap_ip" ):
			case str2int( "wifi_ap_password" ):
			case str2int( "wifi_mode" ):
			case str2int( "wifi_sta_dns" ):
			case str2int( "wifi_sta_gw" ):
			case str2int( "wifi_sta_ip" ):
			case str2int( "wifi_sta_ip_mode" ):
			case str2int( "wifi_sta_password" ):
			case str2int( "windvane_model" ):
				break;
			case str2int( "eth_ip_mode" ):
				x = ( item.value() == "0" ) ? aws_ip_mode::dhcp : aws_ip_mode::fixed;
				break;
			case str2int( "clone_dome_on_rain" ):
			case str2int( "has_bme" ):
			case str2int( "has_dome" ):
			case str2int( "has_gps" ):
			case str2int( "has_mlx" ):
			case str2int( "has_rain_sensor" ):
			case str2int( "has_tsl" ):
			case str2int( "has_ws" ):
			case str2int( "has_wv" ):
				config_items[item.key().c_str()] = ( item.value() == "on" ) ? 1 : 0;
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

AWSNetworkConfig::AWSNetworkConfig( void )
{
	wifi_mode = aws_wifi_mode::ap;
	alpaca_iface = aws_iface::wifi_sta;
	config_iface = aws_iface::wifi_sta;
	pref_iface = aws_iface::wifi_sta;
	eth_ip_mode = aws_ip_mode::dhcp;
	wifi_sta_ip_mode = aws_ip_mode::dhcp;
}

void AWSNetworkConfig::commit_config( JsonDocument &aws_json_config )
{
	pref_iface = aws_json_config.containsKey( "pref_iface" ) ? static_cast<aws_iface>( aws_json_config["pref_iface"].as<byte>() ) : aws_iface::wifi_ap;
	eth_ip_mode = aws_json_config.containsKey( "eth_ip_mode" ) ? static_cast<aws_ip_mode>( aws_json_config["eth_ip_mode"].as<byte>() ) : DEFAULT_ETH_IP_MODE;

	set_parameter( aws_json_config, "eth_ip", &eth_ip, DEFAULT_ETH_IP );
	set_parameter( aws_json_config, "eth_gw", &eth_gw, DEFAULT_ETH_GW );
	set_parameter( aws_json_config, "eth_dns", &eth_dns, DEFAULT_ETH_DNS );

	wifi_mode = aws_json_config.containsKey( "wifi_mode" ) ? static_cast<aws_wifi_mode>( aws_json_config["wifi_mode"].as<byte>() ) : DEFAULT_WIFI_MODE;
		
	set_parameter( aws_json_config, "sta_ssid", &wifi_sta_ssid, DEFAULT_WIFI_STA_SSID );
	set_parameter( aws_json_config, "wifi_sta_password", &wifi_sta_password, DEFAULT_WIFI_STA_PASSWORD );

	wifi_sta_ip_mode = aws_json_config.containsKey( "wifi_sta_ip_mode" ) ? static_cast<aws_ip_mode>( aws_json_config["wifi_sta_ip_mode"].as<byte>() ) : DEFAULT_WIFI_STA_IP_MODE;

	set_parameter( aws_json_config, "wifi_sta_ip", &wifi_sta_ip, DEFAULT_WIFI_STA_IP );
	set_parameter( aws_json_config, "wifi_sta_gw", &wifi_sta_gw, DEFAULT_WIFI_STA_GW );
	set_parameter( aws_json_config, "wifi_sta_dns", &wifi_sta_dns, DEFAULT_WIFI_STA_DNS );

	set_parameter( aws_json_config, "ap_ssid", &wifi_ap_ssid, DEFAULT_WIFI_AP_SSID );
	set_parameter( aws_json_config, "wifi_ap_password", &wifi_ap_password, DEFAULT_WIFI_AP_PASSWORD );
	set_parameter( aws_json_config, "wifi_ap_ip", &wifi_ap_ip, DEFAULT_WIFI_AP_IP );
	set_parameter( aws_json_config, "wifi_ap_gw", &wifi_ap_gw, DEFAULT_WIFI_AP_GW );
	set_parameter( aws_json_config, "wifi_ap_dns", &wifi_ap_dns, DEFAULT_WIFI_AP_DNS );

	config_iface = static_cast<aws_iface>( static_cast<int>( aws_json_config["config_iface"].as<byte>() ));
	
	set_parameter( aws_json_config, "root_ca", &root_ca, DEFAULT_ROOT_CA );
}

aws_iface AWSNetworkConfig::get_alpaca_iface( void )
{
	return alpaca_iface;
}

aws_iface AWSNetworkConfig::get_config_iface( void )
{
	return config_iface;
}

char *AWSNetworkConfig::get_eth_dns( void )
{
	return eth_dns;
}

char *AWSNetworkConfig::get_eth_gw( void )
{
	return eth_gw;
}

char *AWSNetworkConfig::get_eth_ip( void )
{
	return eth_ip;
}

aws_ip_mode AWSNetworkConfig::get_eth_ip_mode( void )
{
	return eth_ip_mode;
}

aws_iface AWSNetworkConfig::get_pref_iface( void )
{
	return pref_iface;
}

char *AWSNetworkConfig::get_root_ca( void )
{
	return root_ca;
}

aws_wifi_mode AWSNetworkConfig::get_wifi_mode( void )
{
	return wifi_mode;
}

char *AWSNetworkConfig::get_wifi_ap_dns( void )
{
	return wifi_ap_dns;
}

char *AWSNetworkConfig::get_wifi_ap_gw( void )
{
	return wifi_ap_gw;
}

char *AWSNetworkConfig::get_wifi_ap_ip( void )
{
	return wifi_ap_ip;
}

char *AWSNetworkConfig::get_wifi_ap_password( void )
{
	return wifi_ap_password;
}

char *AWSNetworkConfig::get_wifi_ap_ssid( void )
{
	return wifi_ap_ssid;
}

char *AWSNetworkConfig::get_wifi_sta_dns( void )
{
	return wifi_sta_dns;
}

char *AWSNetworkConfig::get_wifi_sta_gw( void )
{
	return wifi_sta_gw;
}

char *AWSNetworkConfig::get_wifi_sta_ip( void )
{
	return wifi_sta_ip;
}

aws_ip_mode AWSNetworkConfig::get_wifi_sta_ip_mode( void )
{
	return wifi_sta_ip_mode;
}

char *AWSNetworkConfig::get_wifi_sta_password( void )
{
	return wifi_sta_password;
}

char *AWSNetworkConfig::get_wifi_sta_ssid( void )
{
	return wifi_sta_ssid;
}

bool AWSNetworkConfig::set_parameter( JsonDocument &aws_json_config, const char *config_key, char **config_item, const char *default_value )
{
	if ( aws_json_config.containsKey( config_key ) ) {

		if ( *config_item ) {

			if ( strcmp( *config_item, aws_json_config[config_key] )) {

				if ( *config_item != default_value )
					free( *config_item );
				*config_item = strdup( aws_json_config[config_key] );
				return true;
			}

		} else {

			*config_item = strdup( aws_json_config[config_key] );
			return true;
		}

	} else {

		if ( *config_item && ( *config_item != default_value ))
			free( *config_item );
		*config_item = const_cast<char *>( default_value );
		return true;
	}
	return false;
}
