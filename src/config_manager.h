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

extern const unsigned long MLX_SENSOR;
extern const unsigned long TSL_SENSOR;
extern const unsigned long BME_SENSOR;
extern const unsigned long WIND_VANE_SENSOR;
extern const unsigned long ANEMOMETER_SENSOR;
extern const unsigned long RAIN_SENSOR;
extern const unsigned long GPS_SENSOR;
extern const unsigned long ALL_SENSORS;

enum struct aws_iface : byte {

	wifi_ap,
	wifi_sta,
	eth

};

enum struct aws_wifi_mode : byte {

	sta,
	ap,
	both

};

enum struct aws_pwr_src : byte {

	panel,
	dc12v,
	poe

};

enum struct aws_ip_mode : byte {

	dhcp,
	fixed

};

const bool				DEFAULT_CLOSE_DOME_ON_RAIN			= true;
const int				DEFAULT_CONFIG_PORT					= 80;
const aws_ip_mode		DEFAULT_ETH_IP_MODE					= aws_ip_mode::dhcp;
const uint8_t			DEFAULT_HAS_BME						= 0;
const uint8_t			DEFAULT_HAS_DOME					= 0;
const uint8_t			DEFAULT_HAS_GPS						= 0;
const uint8_t			DEFAULT_HAS_MLX						= 0;
const uint8_t			DEFAULT_HAS_RAIN_SENSOR				= 0;
const uint8_t			DEFAULT_HAS_SC16IS750				= 1;
const uint8_t			DEFAULT_HAS_TSL						= 0;
const uint8_t			DEFAULT_HAS_WS						= 0;
const uint8_t			DEFAULT_HAS_WV						= 0;
const float				DEFAULT_MSAS_CORRECTION				= -0.55;
const aws_iface			DEFAULT_PREF_IFACE					= aws_iface::wifi_ap;
const uint16_t			DEFAULT_RAIN_EVENT_GUARD_TIME		= 60;
const aws_wifi_mode		DEFAULT_WIFI_MODE					= aws_wifi_mode::both;
const aws_ip_mode		DEFAULT_WIFI_STA_IP_MODE			= aws_ip_mode::dhcp;

class AWSNetworkConfig {

	private:

		aws_iface			alpaca_iface		= aws_iface::wifi_ap;
		aws_iface			config_iface		= aws_iface::wifi_ap;
		etl::string<15>		eth_dns;
		etl::string<18>		eth_ip;
		aws_ip_mode			eth_ip_mode			= DEFAULT_ETH_IP_MODE;
		etl::string<15>		eth_gw;
		aws_iface			pref_iface			= DEFAULT_PREF_IFACE;
		etl::string<2300>	root_ca;
		etl::string<15>		wifi_ap_dns;
		etl::string<15>		wifi_ap_gw;
		etl::string<18>		wifi_ap_ip;
		etl::string<32>		wifi_ap_password;
		etl::string<32>		wifi_ap_ssid;
		aws_wifi_mode		wifi_mode			= DEFAULT_WIFI_MODE;
		etl::string<15>		wifi_sta_dns;
		etl::string<15>		wifi_sta_gw;
		etl::string<18>		wifi_sta_ip;
		aws_ip_mode			wifi_sta_ip_mode	= DEFAULT_WIFI_STA_IP_MODE;
		etl::string<32>		wifi_sta_password;
		etl::string<32>		wifi_sta_ssid;

		template <std::size_t sz>
		void 	set_parameter( JsonDocument &, const char *, etl::string<sz> &, const char * );

	public:

							AWSNetworkConfig( void );
		void				commit_config( JsonDocument & );
		aws_iface			get_alpaca_iface( void );
		aws_iface			get_config_iface( void );
		etl::string_view	get_eth_dns( void );
		etl::string_view	get_eth_gw( void );
		etl::string_view	get_eth_ip( void );
		aws_ip_mode			get_eth_ip_mode( void );
		aws_iface			get_pref_iface( void );
		etl::string_view	get_root_ca( void );
		etl::string_view	get_wifi_ap_dns( void );
		etl::string_view	get_wifi_ap_gw( void );
		etl::string_view	get_wifi_ap_ip( void );
		etl::string_view	get_wifi_ap_password( void );
		etl::string_view	get_wifi_ap_ssid( void );
		aws_wifi_mode		get_wifi_mode( void );
		etl::string_view	get_wifi_sta_dns( void );
		etl::string_view	get_wifi_sta_gw( void );
		etl::string_view	get_wifi_sta_ip( void );
		aws_ip_mode			get_wifi_sta_ip_mode( void );
		etl::string_view	get_wifi_sta_password( void );
		etl::string_view	get_wifi_sta_ssid( void );
};

class AWSConfig {

	public:

							AWSConfig( void ) = default;
		bool				can_rollback( void );
		aws_iface			get_alpaca_iface( void );
		uint8_t				get_anemometer_model( void );
		etl::string_view	get_anemometer_model_str( void );
		bool				get_close_dome_on_rain( void );
		aws_iface			get_config_iface( void );
		uint16_t			get_config_port( void );
		etl::string_view	get_eth_dns( void );
		etl::string_view	get_eth_gw( void );
		etl::string_view	get_eth_ip( void );
		aws_ip_mode			get_eth_ip_mode( void );
		bool				get_has_ethernet( void );
		bool 				get_has_bme( void );
		bool				get_has_dome( void );
		bool				get_has_gps( void );
		bool				get_has_mlx( void );
		bool				get_has_rain_sensor( void );
		bool				get_has_sc16is750( void );
		bool				get_has_tsl( void );
		bool				get_has_ws( void );
		bool				get_has_wv( void );
		etl::string_view	get_json_string_config( void );
		float				get_msas_calibration_offset( void );
		etl::string_view	get_pcb_version( void );
		aws_iface			get_pref_iface( void );
		aws_pwr_src			get_pwr_mode( void );
		uint16_t 			get_rain_event_guard_time( void );
		etl::string_view	get_remote_server( void );
		etl::string_view	get_root_ca( void );
		etl::string_view	get_tzname( void );
		etl::string_view	get_url_path( void );
		aws_wifi_mode		get_wifi_mode( void );
		etl::string_view	get_wifi_ap_dns( void );
		etl::string_view	get_wifi_ap_gw( void );
		etl::string_view	get_wifi_ap_ip( void );
		etl::string_view	get_wifi_ap_password( void );
		etl::string_view	get_wifi_ap_ssid( void );
		etl::string_view	get_wifi_sta_dns( void );
		etl::string_view	get_wifi_sta_gw( void );
		etl::string_view	get_wifi_sta_ip( void );
		aws_ip_mode			get_wifi_sta_ip_mode( void );
		etl::string_view	get_wifi_sta_password( void );
		etl::string_view	get_wifi_sta_ssid( void );
		uint8_t				get_wind_vane_model( void );
		etl::string_view	get_wind_vane_model_str( void );
		bool 				load( bool );
		void				reset_parameter( const char * );
		bool				rollback( void );
		bool				save_runtime_configuration( JsonVariant & );
		bool 				update( JsonVariant &json );
		
	private:

		uint8_t 			anemometer_model		= 255;
		bool				close_dome_on_rain		= true;
		uint16_t			config_port				= DEFAULT_CONFIG_PORT;
		bool				debug_mode				= false;
		uint32_t			devices					= 0;
		bool				initialised				= false;
		float				msas_calibration_offset	= 0.F;
		AWSNetworkConfig	network_config;
		etl::string<8>		pcb_version;
		aws_pwr_src			pwr_mode				= aws_pwr_src::dc12v;
		uint16_t			rain_event_guard_time	= 60;
		etl::string<32>		remote_server;
		etl::string<32>		tzname;
		etl::string<16>		url_path;
		uint8_t				wind_vane_model			= 255;

		bool	read_config( void );
		bool	read_file( const char *, JsonDocument & );
		bool 	read_hw_info_from_nvs( void );
		template <std::size_t sz>
		void 	set_parameter( JsonDocument &, const char *, etl::string<sz> &, const char * );
		bool	verify_entries( JsonVariant & );
};

#endif
