/*
  	config_manager.h

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

#pragma once
#ifndef _config_manager_H
#define _config_manager_H

#include <ArduinoJson.h>

#include "device.h"
enum struct aws_iface : int {

	wifi_ap,
	wifi_sta,
	eth

};

enum struct aws_wifi_mode : int {

	sta,
	ap,
	both

};

enum struct aws_pwr_src : int {

	panel,
	dc12v,
	poe

};

enum struct aws_ip_mode : int {

	dhcp,
	fixed

};

const int				DEFAULT_CONFIG_PORT						= 80;
const aws_ip_mode		DEFAULT_ETH_IP_MODE						= aws_ip_mode::dhcp;
const uint8_t			DEFAULT_HAS_BME							= 0;
const uint8_t			DEFAULT_HAS_DOME						= 0;
const uint8_t			DEFAULT_HAS_GPS							= 0;
const uint8_t			DEFAULT_HAS_MLX							= 0;
const uint8_t			DEFAULT_HAS_RAIN_SENSOR					= 0;
const uint8_t			DEFAULT_HAS_SC16IS750					= 1;
const uint8_t			DEFAULT_HAS_TSL							= 0;
const uint8_t			DEFAULT_HAS_WS							= 0;
const uint8_t			DEFAULT_HAS_WV							= 0;
const bool				DEFAULT_LOOKOUT_ENABLED					= false;
const float				DEFAULT_MSAS_CORRECTION					= -0.55;
const aws_iface			DEFAULT_PREF_IFACE						= aws_iface::wifi_ap;

const int				DEFAULT_K1								= 33;
const int				DEFAULT_K2								= 0;
const int				DEFAULT_K3								= 8;
const int				DEFAULT_K4								= 100;
const int				DEFAULT_K5								= 100;
const int				DEFAULT_K6								= 0;
const int				DEFAULT_K7								= 0;
const int				DEFAULT_CC_AWS_OVERCAST					= -20;
const int				DEFAULT_CC_AWS_CLOUDY					= -25;
const int				DEFAULT_CC_AAG_OVERCAST					= -15;
const int				DEFAULT_CC_AAG_CLOUDY					= -20;

const uint16_t			DEFAULT_RAIN_EVENT_GUARD_TIME			= 60;

const bool				DEFAULT_SAFE_CLOUD_COVERAGE_1_ACTIVE	= false;
const bool				DEFAULT_SAFE_CLOUD_COVERAGE_2_ACTIVE	= false;
const uint16_t			DEFAULT_SAFE_CLOUD_COVERAGE_1_DELAY		= 0;
const uint16_t			DEFAULT_SAFE_CLOUD_COVERAGE_2_DELAY		= 0;
const uint16_t			DEFAULT_SAFE_CLOUD_COVERAGE_1_MAX		= 0;
const uint16_t			DEFAULT_SAFE_CLOUD_COVERAGE_2_MAX		= 0;
const bool				DEFAULT_SAFE_RAIN_INTENSITY_ACTIVE		= false;
const uint16_t			DEFAULT_SAFE_RAIN_INTENSITY_DELAY		= 0;
const uint16_t			DEFAULT_SAFE_RAIN_INTENSITY_MAX			= 0;
const bool				DEFAULT_SAFE_WIND_SPEED_ACTIVE			= false;
const uint16_t			DEFAULT_SAFE_WIND_SPEED_DELAY			= 0;
const uint16_t			DEFAULT_SAFE_WIND_SPEED_MAX				= 0;

const bool				DEFAULT_UNSAFE_CLOUD_COVERAGE_1_ACTIVE	= false;
const bool				DEFAULT_UNSAFE_CLOUD_COVERAGE_2_ACTIVE	= false;
const uint16_t			DEFAULT_UNSAFE_CLOUD_COVERAGE_1_DELAY	= 0;
const uint16_t			DEFAULT_UNSAFE_CLOUD_COVERAGE_2_DELAY	= 0;
const uint16_t			DEFAULT_UNSAFE_CLOUD_COVERAGE_1_MAX		= 0;
const uint16_t			DEFAULT_UNSAFE_CLOUD_COVERAGE_2_MAX		= 0;
const bool				DEFAULT_UNSAFE_CLOUD_COVERAGE_1_MISSING	= false;	// if not checked, the key will not be in the submitted json, ditto for other similar parameters
const bool				DEFAULT_UNSAFE_CLOUD_COVERAGE_2_MISSING	= false;
const bool				DEFAULT_UNSAFE_RAIN_EVENT_ACTIVE		= true;
const bool				DEFAULT_UNSAFE_RAIN_EVENT_MISSING		= true;
const bool				DEFAULT_UNSAFE_RAIN_INTENSITY_ACTIVE	= false;
const uint16_t			DEFAULT_UNSAFE_RAIN_INTENSITY_MAX		= 0;
const bool				DEFAULT_UNSAFE_RAIN_INTENSITY_MISSING	= false;
const bool				DEFAULT_UNSAFE_WIND_SPEED_1_ACTIVE		= false;
const bool				DEFAULT_UNSAFE_WIND_SPEED_2_ACTIVE		= false;
const uint16_t			DEFAULT_UNSAFE_WIND_SPEED_1_DELAY		= 0;
const uint16_t			DEFAULT_UNSAFE_WIND_SPEED_2_DELAY		= 0;
const uint16_t			DEFAULT_UNSAFE_WIND_SPEED_1_MAX			= 0;
const uint16_t			DEFAULT_UNSAFE_WIND_SPEED_2_MAX			= 0;
const bool				DEFAULT_UNSAFE_WIND_SPEED_1_MISSING		= false;
const bool				DEFAULT_UNSAFE_WIND_SPEED_2_MISSING		= false;

const aws_wifi_mode		DEFAULT_WIFI_MODE						= aws_wifi_mode::both;
const aws_ip_mode		DEFAULT_WIFI_STA_IP_MODE				= aws_ip_mode::dhcp;

const bool				DEFAULT_DATA_PUSH						= true;
const uint16_t			DEFAULT_PUSH_FREQ						= 300;

const bool				DEFAULT_DISCORD_ENABLED					= false;
const char				DEFAULT_DISCORD_WEBHOOK[]				= "";

const char				DEFAULT_OTA_URL[]						= "https://www.datamancers.net/images/AWS.json";

class AWSConfig {

	public:

								AWSConfig( void );
		bool					can_rollback( void );
		void					factory_reset( void );
		etl::string_view		get_anemometer_model_str( void );
		uint32_t				get_fs_free_space( void );
		template <typename T>
		T 						get_lookout_safe_parameter( const char * );
		template <typename T>
		T 						get_lookout_unsafe_parameter( const char * );
		template <typename T>
		T 						get_parameter( const char * );
		bool					get_has_device( aws_device_t );
		etl::string_view		get_json_string_config( void );
		etl::string_view		get_ota_sha256( void );
		etl::string_view		get_pcb_version( void );
		aws_pwr_src				get_pwr_mode( void );
		etl::string_view		get_root_ca( void );
		etl::string_view		get_wind_vane_model_str( void );
		bool 					load( bool );
		void					reset_parameter( const char * );
		bool					rollback( void );
		bool					save_runtime_configuration( JsonVariant & );
		bool 					update( JsonVariant &json );

	private:

		const size_t		MAX_CONFIG_FILE_SIZE	= 2048;
		bool				debug_mode				= false;
		aws_device_t		devices					= aws_device_t::NO_SENSOR;
		uint32_t			fs_free_space			= 0;
		bool				initialised				= false;
		DynamicJsonDocument	*json_config;
		etl::string<64>		ota_sha256;
		etl::string<8>		pcb_version;
		aws_pwr_src			pwr_mode				= aws_pwr_src::dc12v;
		etl::string<4096>	root_ca;
		
		template <typename T>
		T 		get_aag_parameter( const char * );
		void	list_files( void );
		bool	read_config( void );
		bool	read_file( const char * );
		bool	read_hw_info_from_nvs( void );
		void	read_root_ca( void );
		void	set_missing_lookout_parameters_to_default_values( void );
		void	set_missing_lookout_safe_parameters_to_default_values( void );
		void	set_missing_lookout_unsafe_parameters_to_default_values( void );
		void	set_missing_network_parameters_to_default_values( void );
		void	set_missing_parameters_to_default_values( void );
		void	set_root_ca( JsonVariant & );
		void	update_fs_free_space( void );
		bool	verify_entries( JsonVariant & );
};

//
// Credits to: https://stackoverflow.com/a/16388610
//
constexpr unsigned int str2int( const char* str, int h = 0 )
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

template <typename T>
T AWSConfig::get_lookout_safe_parameter( const char *key )
{
	switch( str2int( key )) {
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
			return (*json_config)[key].as<T>();
	}
	Serial.printf( "[CONFIGMNGR] [ERROR]: Unknown parameter [%s]\n", key );
	return 0;
}

template <typename T>
T AWSConfig::get_lookout_unsafe_parameter( const char *key )
{
	switch( str2int( key )) {
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
			return (*json_config)[key].as<T>();
	}
	Serial.printf( "[CONFIGMNGR] [ERROR]: Unknown parameter [%s]\n", key );
	return 0;
}

template <typename T>
T AWSConfig::get_aag_parameter( const char *key )
{
	switch( str2int( key )) {
		case str2int( "k1" ):
		case str2int( "k2" ):
		case str2int( "k3" ):
		case str2int( "k4" ):
		case str2int( "k5" ):
		case str2int( "k6" ):
		case str2int( "k7" ):
		case str2int( "cc_aws_cloudy" ):
		case str2int( "cc_aws_overcast" ):
		case str2int( "cc_aag_cloudy" ):
		case str2int( "cc_aag_overcast" ):
			return (*json_config)[key].as<T>();
	}
	Serial.printf( "[CONFIGMNGR] [ERROR]: Unknown parameter [%s]\n", key );
	return 0;
}

template <typename T>
T AWSConfig::get_parameter( const char *key )
{
	if ( !strncmp( key, "unsafe_", 7 ))
		return get_lookout_unsafe_parameter<T>( key );

	if ( !strncmp( key, "safe_", 5 ))
		return get_lookout_safe_parameter<T>( key );

	if (( *key == 'k' ) || ( !strncmp( key, "cc_", 3  )))
		return get_aag_parameter<T>( key );

	switch( str2int( key )) {

		case str2int( "alpaca_iface" ):
		case str2int( "anemometer_model" ):
		case str2int( "cloud_coverage_formula" ):
		case str2int( "config_iface" ):
		case str2int( "config_port" ):
		case str2int( "wind_vane_model" ):
			return ( json_config->containsKey( key ) ? (*json_config)[key].as<T>() : 0 );

		case str2int( "automatic_updates" ):
		case str2int( "data_push" ):
		case str2int( "discord_enabled" ):
		case str2int( "discord_wh" ):
		case str2int( "eth_dns" ):
		case str2int( "eth_gw" ):
		case str2int( "eth_ip" ):
		case str2int( "eth_ip_mode" ):
		case str2int( "lookout_enabled" ):
		case str2int( "msas_calibration_offset" ):
		case str2int( "ota_url" ):
		case str2int( "pref_iface" ):
		case str2int( "push_freq" ):
		case str2int( "rain_event_guard_time" ):
		case str2int( "remote_server" ):
		case str2int( "tzname" ):
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
			return (*json_config)[key].as<T>();

	}
	Serial.printf( "[CONFIGMNGR] [ERROR]: Unknown parameter [%s]\n", key );
	return 0;
}

#endif
