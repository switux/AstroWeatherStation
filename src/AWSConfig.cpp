/*	
  	AWSConfig.cpp
  	
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
#define ASYNCWEBSERVER_REGEX	1

#include <Ethernet.h>
#include <SSLClient.h>
#include <time.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

#include "defaults.h"
#include "SC16IS750.h"
#include "AWSGPS.h"
#include "AstroWeatherStation.h"
#include "SQM.h"
#include "AWSSensorManager.h"
#include "AWSWeb.h"
#include "dome.h"
#include "alpaca.h"
#include "AWSConfig.h"
#include "AWS.h"

extern AstroWeatherStation station;

const char	*pwr_mode_str[3] = {
	"Solar panel",
	"12V DC",
	"POE"	
};

//
// Credits to: https://stackoverflow.com/a/16388610
//
constexpr unsigned int str2int( const char* str, int h = 0 )
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

AWSConfig::AWSConfig( void )
{
	ap_ssid = NULL;
	wifi_ap_password = NULL;
	rain_event_guard_time = 60;
	sta_ssid = wifi_sta_password = remote_server = url_path = root_ca = tzname = eth_dns = eth_ip = eth_gw = wifi_sta_dns = wifi_sta_ip = wifi_sta_gw = wifi_ap_dns = wifi_ap_ip = wifi_ap_gw = NULL;
	wifi_mode = ap;
}

aws_iface_t	AWSConfig::get_alpaca_iface( void )
{
	return alpaca_iface;
}

uint8_t *AWSConfig::get_anemometer_cmd( void )
{
	return anemometer_cmd;
}

uint16_t AWSConfig::get_anemometer_com_speed( void )
{
	return anemometer_com_speed;
}

uint8_t AWSConfig::get_anemometer_model( void )
{
	return anemometer_model;
}

const char *AWSConfig::get_anemometer_model_str( void )
{
	return _anemometer_model[ anemometer_model ];
}

char *AWSConfig::get_ap_ssid( void )
{
	return ap_ssid;
}

bool AWSConfig::get_close_dome_on_rain( void )
{
	return close_dome_on_rain;
}

aws_iface_t	AWSConfig::get_config_iface( void )
{
	return config_iface;
}

uint16_t AWSConfig::get_config_port( void )
{
	return config_port;
}

char *AWSConfig::get_eth_dns( void )
{
	return eth_dns;
}

char *AWSConfig::get_eth_gw( void )
{
	return eth_gw;
}

char *AWSConfig::get_eth_ip( void )
{
	return eth_ip;
}

aws_ip_mode_t AWSConfig::get_eth_ip_mode( void )
{
	return eth_ip_mode;
}

bool AWSConfig::get_has_bme( void )
{
	return has_bme;
}

bool AWSConfig::get_has_dome( void )
{
	return has_dome;
}

bool AWSConfig::get_has_ethernet( void )
{
	return has_ethernet;
}

bool AWSConfig::get_has_gps( void )
{
	return has_gps;
}

bool AWSConfig::get_has_mlx( void )
{
	return has_mlx;
}

bool AWSConfig::get_has_rg9( void )
{
	return has_rg9;
}

bool AWSConfig::get_has_sc16is750( void )
{
	return has_sc16is750;
}

bool AWSConfig::get_has_tsl( void )
{
	return has_tsl;
}

bool AWSConfig::get_has_ws( void )
{
	return has_ws;
}

bool AWSConfig::get_has_wv( void )
{
	return has_wv;
}

char *AWSConfig::get_json_string_config( void )
{
	DynamicJsonDocument	aws_json_config(1024);
	char *json_string = (char *)malloc(1024);
	char buf[64];
		
	aws_json_config["tzname"] = tzname;
	aws_json_config["pref_iface"] = pref_iface;
    aws_json_config["eth_ip_mode"] = eth_ip_mode;
	if ( eth_ip_mode == dhcp ) {

		sprintf( buf, "%s/%d", station.get_eth_ip()->toString().c_str(), station.get_eth_cidr_prefix() );
		aws_json_config["eth_ip"] = buf;
		sprintf( buf, "%s",  station.get_eth_gw()->toString().c_str() );
		aws_json_config["eth_gw"] = buf;
		sprintf( buf, "%s",  station.get_eth_dns()->toString().c_str() );
		aws_json_config["eth_dns"] = buf;

	} else {

		aws_json_config["eth_ip"] = eth_ip;
		aws_json_config["eth_gw"] = eth_gw;
		aws_json_config["eth_dns"] = eth_dns;

	}
	aws_json_config["sta_ssid"] = sta_ssid;
	aws_json_config["wifi_sta_password"] = wifi_sta_password;
	aws_json_config["wifi_sta_ip_mode"] = wifi_sta_ip_mode;

	if ( wifi_sta_ip_mode == dhcp ) {

		sprintf( buf, "%s/%d", station.get_sta_ip()->toString().c_str(), station.get_sta_cidr_prefix() );
		aws_json_config["wifi_sta_ip"] = buf;
		sprintf( buf, "%s",  station.get_sta_gw()->toString().c_str() );
		aws_json_config["wifi_sta_gw"] = buf;
		sprintf( buf, "%s",  station.get_sta_dns()->toString().c_str() );
		aws_json_config["wifi_sta_dns"] = buf;
		
	} else {

		aws_json_config["wifi_sta_ip"] = wifi_sta_ip;
		aws_json_config["wifi_sta_gw"] = eth_gw;
		aws_json_config["wifi_sta_dns"] = eth_dns;

	}

	aws_json_config["ap_ssid"] = ap_ssid;
	aws_json_config["wifi_ap_password"] = wifi_ap_password;
	aws_json_config["wifi_ap_ip"] = wifi_ap_ip;
	aws_json_config["wifi_ap_gw"] = wifi_ap_gw;

	aws_json_config["remote_server"] = remote_server;
	aws_json_config["url_path"] = url_path;
	aws_json_config["has_bme"] = has_bme;
	aws_json_config["has_tsl"] = has_tsl;
	aws_json_config["has_mlx"] = has_mlx;
	aws_json_config["has_ws"] = has_ws;
	aws_json_config["anemometer_model"] = anemometer_model;
	aws_json_config["has_wv"] = has_wv;
	aws_json_config["windvane_model"] = wind_vane_model;
	aws_json_config["has_rg9"] = has_rg9;
	aws_json_config["rain_event_guard_time"] = rain_event_guard_time;
	aws_json_config["has_gps"] = has_gps;
	aws_json_config["wifi_mode"] = wifi_mode;
	aws_json_config["alpaca_iface"] = alpaca_iface;
	aws_json_config["config_iface"] = config_iface;

	aws_json_config["has_sc16is750" ] = has_sc16is750;
	
	aws_json_config["has_dome"] = has_dome;
	aws_json_config["close_dome_on_rain"] = close_dome_on_rain;
	
	aws_json_config["has_ethernet"] = has_ethernet;
	
	if ( serializeJson( aws_json_config, json_string, 1024 ) >= 1024 ) {

		Serial.printf( "[ERROR] Reached configuration string limit. Please contact support\n" );
		free( json_string );
		return NULL;
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

aws_iface_t AWSConfig::get_pref_iface( void )
{
	return pref_iface;
}

aws_pwr_src_t AWSConfig::get_pwr_mode( void )
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
	return root_ca;
}

char *AWSConfig::get_sta_dns( void )
{
	return wifi_sta_dns;
}

char *AWSConfig::get_sta_gw( void )
{
	return wifi_sta_gw;
}

char *AWSConfig::get_sta_ip( void )
{
	return wifi_sta_ip;
}

char *AWSConfig::get_sta_ssid( void )
{
	return sta_ssid;
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
	return wifi_ap_dns;
}

char *AWSConfig::get_wifi_ap_gw( void )
{
	return wifi_ap_gw;
}

char *AWSConfig::get_wifi_ap_ip( void )
{
	return wifi_ap_ip;
}

aws_wifi_mode_t AWSConfig::get_wifi_mode( void )
{
	return wifi_mode;
}

char *AWSConfig::get_wifi_ap_password( void )
{
	return wifi_ap_password;
}

char *AWSConfig::get_wifi_sta_dns( void )
{
	return wifi_sta_dns;
}

char *AWSConfig::get_wifi_sta_gw( void )
{
	return wifi_sta_gw;
}

char *AWSConfig::get_wifi_sta_ip( void )
{
	return wifi_sta_ip;
}

aws_ip_mode_t AWSConfig::get_wifi_sta_ip_mode( void )
{
	return wifi_sta_ip_mode;
}

char *AWSConfig::get_wifi_sta_password( void )
{
	return wifi_sta_password;
}

uint8_t *AWSConfig::get_wind_vane_cmd( void )
{
	return wind_vane_cmd;
}

uint16_t AWSConfig::get_wind_vane_com_speed( void )
{
	return wind_vane_com_speed;
}

uint8_t AWSConfig::get_wind_vane_model( void )
{
	return wind_vane_model;
}

const char *AWSConfig::get_wind_vane_model_str( void )
{
	return _windvane_model[ wind_vane_model ];
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
	aws_ip_mode_t	t;
	aws_iface_t		u;
	aws_wifi_mode_t v;
	float			w;
	bool			x;
	uint8_t			y;
	uint16_t		z;
		
	if ( !read_file( "/aws.conf", aws_json_config ) ) {
		
		if ( !read_file( "/aws.conf.dfl", aws_json_config )) {
			
			Serial.printf( "\n[ERROR] Could not read config file.\n" );
			return false;
		}
		Serial.printf( "[INFO] Using minimal/factory config file.\n" );
	}

	if ( (!aws_json_config.containsKey( "pwr_mode" )) || (!aws_json_config.containsKey( "pcb_version" )) ) {

		Serial.printf( "[PANIC] Could not find 'pwr_mode' or 'pcb_version' in configuration file!\n" );
		return false;
	}

	pcb_version = strdup( aws_json_config["pcb_version"] );
	pwr_mode = aws_json_config["pwr_mode"];
	
	u = aws_json_config.containsKey( "pref_iface" ) ? aws_json_config["pref_iface"] : wifi_ap;
	if ( pref_iface != u )
		pref_iface = u;

	t = aws_json_config.containsKey( "eth_ip_mode" ) ? aws_json_config["eth_ip_mode"] : DEFAULT_ETH_IP_MODE;
	if ( eth_ip_mode != t )
		eth_ip_mode = t;

	set_parameter( aws_json_config, "eth_ip", &eth_ip, DEFAULT_ETH_IP );
	set_parameter( aws_json_config, "eth_gw", &eth_gw, DEFAULT_ETH_GW );
	set_parameter( aws_json_config, "eth_dns", &eth_dns, DEFAULT_ETH_DNS );

	v = aws_json_config.containsKey( "wifi_mode" ) ? aws_json_config["wifi_mode"] : DEFAULT_WIFI_MODE;
	if ( wifi_mode != v )
		wifi_mode = v;
		
	set_parameter( aws_json_config, "sta_ssid", &sta_ssid, DEFAULT_STA_SSID );
	set_parameter( aws_json_config, "wifi_sta_password", &wifi_sta_password, DEFAULT_WIFI_STA_PASSWORD );

	t = aws_json_config.containsKey( "wifi_sta_ip_mode" ) ? aws_json_config["wifi_sta_ip_mode"] : DEFAULT_WIFI_STA_IP_MODE;
	if ( wifi_sta_ip_mode != t )
		wifi_sta_ip_mode = t;

	set_parameter( aws_json_config, "wifi_sta_ip", &wifi_sta_ip, DEFAULT_WIFI_STA_IP );
	set_parameter( aws_json_config, "wifi_sta_gw", &wifi_sta_gw, DEFAULT_WIFI_STA_GW );
	set_parameter( aws_json_config, "wifi_sta_dns", &wifi_sta_dns, DEFAULT_WIFI_STA_DNS );

	set_parameter( aws_json_config, "ap_ssid", &ap_ssid, DEFAULT_AP_SSID );
	set_parameter( aws_json_config, "wifi_ap_password", &wifi_ap_password, DEFAULT_WIFI_AP_PASSWORD );
	set_parameter( aws_json_config, "wifi_ap_ip", &wifi_ap_ip, DEFAULT_WIFI_AP_IP );
	set_parameter( aws_json_config, "wifi_ap_gw", &wifi_ap_gw, DEFAULT_WIFI_AP_GW );
	set_parameter( aws_json_config, "wifi_ap_dns", &wifi_ap_dns, DEFAULT_WIFI_AP_DNS );

	alpaca_iface = aws_json_config["alpaca_iface"];
	config_iface = aws_json_config["config_iface"];
	
	has_ethernet = aws_json_config.containsKey( "has_ethernet" ) ? ( aws_json_config["has_ethernet"] == 1 ) : DEFAULT_HAS_ETHERNET;

	y = aws_json_config.containsKey( "windvane_model" ) ? aws_json_config["windvane_model"] : 0;
	if ( wind_vane_model != y )
		wind_vane_model = y;

	y = aws_json_config.containsKey( "anemometer_model" ) ? aws_json_config["anemometer_model"] : 0;
	if ( anemometer_model != y )
		anemometer_model = y;

	y = aws_json_config.containsKey( "config_port" ) ? aws_json_config["config_port"] : DEFAULT_CONFIG_PORT;
	if ( config_port != y )
		config_port = y;

	set_parameter( aws_json_config, "remote_server", &remote_server, DEFAULT_SERVER );
	set_parameter( aws_json_config, "url_path", &url_path, DEFAULT_URL_PATH );
	set_parameter( aws_json_config, "tzname", &tzname, DEFAULT_TZNAME );
	set_parameter( aws_json_config, "root_ca", &root_ca, DEFAULT_ROOT_CA );

	w = aws_json_config.containsKey( "msas_calibration_offset" ) ? atof( aws_json_config["msas_calibration_offset"] ) : DEFAULT_MSAS_CORRECTION;
	if ( msas_calibration_offset != w )
		msas_calibration_offset = w;

	x = aws_json_config.containsKey( "close_dome_on_rain" ) ? ( aws_json_config["close_dome_on_rain"] == 1 ) : DEFAULT_CLOSE_DOME_ON_RAIN;
	if ( close_dome_on_rain != x )
		close_dome_on_rain = x;

	x = aws_json_config.containsKey( "has_dome" ) ? ( aws_json_config["has_dome"] == 1 ) : DEFAULT_HAS_DOME;
	if ( has_dome != x )
		has_dome = x;

	x = aws_json_config.containsKey( "has_bme" ) ? ( aws_json_config["has_bme"] == 1 ) : DEFAULT_HAS_BME;
	if ( has_bme != x )
		has_bme = x;
	
	x = aws_json_config.containsKey( "has_gps" ) ? ( aws_json_config["has_gps"] == 1 ) : DEFAULT_HAS_GPS;
	if ( has_gps != x )
		has_gps = x;

	x = aws_json_config.containsKey( "has_mlx" ) ? ( aws_json_config["has_mlx"] == 1 ) : DEFAULT_HAS_MLX;
	if ( has_mlx != x )
		has_mlx = x;

	x = aws_json_config.containsKey( "has_rg9" ) ? ( aws_json_config["has_rg9"] == 1 ) : DEFAULT_HAS_RG9;
	if ( has_rg9 != x )
		has_rg9 = x;

	z = aws_json_config.containsKey( "rain_event_guard_time" ) ? atof( aws_json_config["rain_event_guard_time"] ) : DEFAULT_RAIN_EVENT_GUARD_TIME;
	if ( rain_event_guard_time != z )
		rain_event_guard_time = z;
		
	x = aws_json_config.containsKey( "has_sc16is750" ) ? ( aws_json_config["has_sc16is750"] == 1 ) : DEFAULT_HAS_SC16IS750;
	if ( has_sc16is750 != x )
		has_sc16is750 = x;

	x = aws_json_config.containsKey( "has_tsl" ) ? ( aws_json_config["has_tsl"] == 1 ) : DEFAULT_HAS_TSL;
	if ( has_tsl != x )
		has_tsl = x;

	x = aws_json_config.containsKey( "has_ws" ) ? ( aws_json_config["has_ws"] == 1 ) : DEFAULT_HAS_WS;
	if ( has_ws != x )
		has_ws = x;

	x = aws_json_config.containsKey( "has_wv" ) ? ( aws_json_config["has_wv"] == 1 ) : DEFAULT_HAS_WV;
	if ( has_wv != x )
		has_wv = x;

	return true;
}

bool AWSConfig::read_file( const char *filename, JsonDocument &_json_config )
{
	char	*json_string;
	int		s = 0;

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

	json_string = (char *)malloc( s );
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

bool AWSConfig::rollback()
{
	uint8_t	buf[ 4096 ];
	size_t	i;

	Serial.printf( "[INFO] Rolling back last submitted configuration.\n" );
  
	if ( !SPIFFS.begin( true )) {
		
		Serial.printf( "[ERROR] Could not open filesystem.\n" );
		return false;
	}
	File file = SPIFFS.open( "/aws.conf", FILE_WRITE );
	File filebak = SPIFFS.open( "/aws.conf.bak" );
	
	while( filebak.available() ) {
    
		i = filebak.readBytes( (char *)buf, 4096 );
		file.write( buf, i );
	}
 
	file.close();
	filebak.close();
	SPIFFS.remove( "/aws.conf.bak" );
	Serial.printf( "[INFO] Rollback successful.\n" );
	return true;
}

bool AWSConfig::save_runtime_configuration( JsonVariant &json_config )
{
	uint8_t	buf[ 4096 ];
	size_t	i;

	if ( !verify_entries( json_config ))
		return false;

	Serial.printf( "[INFO] Saving submitted configuration.\n" );

	if ( !SPIFFS.begin( true )) {
		
		Serial.printf( "[ERROR] Could not open filesystem.\n" );
		return false;
	}
	File file = SPIFFS.open( "/aws.conf" );
	File filebak = SPIFFS.open( "/aws.conf.bak", FILE_WRITE );
	
	while( file.available() ) {
		i = file.readBytes( (char *)buf, 4096 );
		filebak.write( buf, i );
	}
	file.close();
	filebak.close();
	
	file = SPIFFS.open( "/aws.conf", FILE_WRITE );
	serializeJson( json_config, file );
	file.close();
	Serial.printf( "[INFO] Save successful.\n" );
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
		*config_item = (char *)default_value;
		return true;
	}
	return false;
}

bool AWSConfig::verify_entries( JsonVariant &proposed_config )
{
	// proposed_config will de facto be modified in this function as
	// config_items is only a way of presenting the items in proposed_config

	JsonObject config_items = proposed_config.as<JsonObject>();
	byte	x = dhcp;
	
	for( JsonPair item : config_items ) {

		switch( str2int( item.key().c_str(), 0 )) {

			case str2int("alpaca_iface", 0 ):
			case str2int("anemometer_model", 0 ):
			case str2int("ap_ssid", 0  ):
			case str2int("eth_dns", 0 ):
			case str2int("eth_gw", 0 ):
			case str2int("eth_ip", 0 ):
			case str2int("pref_iface", 0 ):
			case str2int("rain_event_guard_time", 0 ):
			case str2int("remote_server", 0  ):
			case str2int("root_ca", 0  ):
			case str2int("sta_ssid", 0  ):
			case str2int("tzname", 0 ):
			case str2int("url_path", 0 ):
			case str2int("wifi_ap_dns", 0 ):
			case str2int("wifi_ap_gw", 0 ):
			case str2int("wifi_ap_ip", 0 ):
			case str2int("wifi_ap_password", 0 ):
			case str2int("wifi_mode", 0 ):
			case str2int("wifi_sta_dns", 0 ):
			case str2int("wifi_sta_gw", 0 ):
			case str2int("wifi_sta_ip", 0 ):
			case str2int("wifi_sta_ip_mode", 0 ):
			case str2int("wifi_sta_password", 0 ):
			case str2int("windvane_model", 0 ):
				break;
			case str2int("eth_ip_mode", 0):
				x = ( item.value() == "0" ) ? dhcp : fixed;
				break;
			case str2int("clone_dome_on_rain", 0):
			case str2int("has_bme", 0):
			case str2int("has_dome", 0):
			case str2int("has_gps", 0):
			case str2int("has_mlx", 0):
			case str2int("has_rg9", 0):
			case str2int("has_tsl", 0):
			case str2int("has_ws", 0):
			case str2int("has_wv", 0):
				config_items[item.key().c_str()] = ( item.value() == "on" ) ? 1 : 0;
				break;
			default:
				Serial.printf( "[ERROR] Unknown configuration key [%s]\n",  item.key().c_str() );
				return false;
		}
	}

	if ( x == dhcp ) {

		config_items.remove("eth_ip");
		config_items.remove("eth_gw");
		config_items.remove("eth_dns");
	}

	// MUST NOT BE CHANGED AS IT IS HARDWARE MODEL DEPENDENT
	config_items["pcb_version"] = pcb_version;
	config_items["pwr_mode"] = pwr_mode;
	config_items["has_ethernet"] = has_ethernet;
	config_items["has_sc16is750"] = has_sc16is750;

	return true;	
}
