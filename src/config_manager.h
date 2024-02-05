/*
  	config.h

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

// flawfinder: ignore
extern const char *_windvane_model[3];

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

class AWSNetworkConfig {

	private:

		aws_iface		alpaca_iface;
		aws_iface		config_iface;
		char			*eth_dns;
		char			*eth_ip;
		aws_ip_mode		eth_ip_mode;
		char			*eth_gw;
		aws_iface		pref_iface;
		char			*root_ca;
		char			*wifi_ap_dns;
		char			*wifi_ap_gw;
		char			*wifi_ap_ip;
		char			*wifi_ap_password;
		char			*wifi_ap_ssid;
		aws_wifi_mode	wifi_mode;
		char			*wifi_sta_dns;
		char			*wifi_sta_gw;
		char			*wifi_sta_ip;
		aws_ip_mode		wifi_sta_ip_mode;
		char			*wifi_sta_password;
		char			*wifi_sta_ssid;

		bool	set_parameter( JsonDocument &, const char *, char **, const char * );

	public:

						AWSNetworkConfig( void );
		void			commit_config( JsonDocument & );
		aws_iface		get_alpaca_iface( void );
		aws_iface		get_config_iface( void );
		char			*get_eth_dns( void );
		char			*get_eth_gw( void );
		char 			*get_eth_ip( void );
		aws_ip_mode		get_eth_ip_mode( void );
		aws_iface		get_pref_iface( void );
		char            *get_root_ca( void );
		char			*get_wifi_ap_dns( void );
		char			*get_wifi_ap_gw( void );
		char			*get_wifi_ap_ip( void );
		char			*get_wifi_ap_password( void );
		char			*get_wifi_ap_ssid( void );
		aws_wifi_mode	get_wifi_mode( void );
		char			*get_wifi_sta_dns( void );
		char            *get_wifi_sta_gw( void );
		char            *get_wifi_sta_ip( void );
		aws_ip_mode		get_wifi_sta_ip_mode( void );
		char			*get_wifi_sta_password( void );
		char            *get_wifi_sta_ssid( void );
};

class AWSConfig {

	public:

						AWSConfig( void );
		bool			can_rollback( void );
		aws_iface		get_alpaca_iface( void );
		uint8_t			get_anemometer_model( void );
		const char *	get_anemometer_model_str( void );
		bool			get_close_dome_on_rain( void );
		aws_iface		get_config_iface( void );
		uint16_t		get_config_port( void );
		char *			get_eth_dns( void );
		char *			get_eth_gw( void );
		char *			get_eth_ip( void );
		aws_ip_mode		get_eth_ip_mode( void );
		bool			get_has_ethernet( void );
		bool 			get_has_bme( void );
		bool			get_has_dome( void );
		bool			get_has_gps( void );
		bool			get_has_mlx( void );
		bool			get_has_rain_sensor( void );
		bool			get_has_sc16is750( void );
		bool			get_has_tsl( void );
		bool			get_has_ws( void );
		bool			get_has_wv( void );
		char *			get_json_string_config( void );
		float			get_msas_calibration_offset( void );
		char *			get_pcb_version( void );
		aws_iface		get_pref_iface( void );
		aws_pwr_src		get_pwr_mode( void );
		uint16_t 		get_rain_event_guard_time( void );
		char            *get_remote_server( void );
		char            *get_root_ca( void );
		char            *get_tzname( void );
		char            *get_url_path( void );
		aws_wifi_mode	get_wifi_mode( void );
		char *			get_wifi_ap_dns( void );
		char *			get_wifi_ap_gw( void );
		char *			get_wifi_ap_ip( void );
		char *			get_wifi_ap_password( void );
		char			*get_wifi_ap_ssid( void );
		char *			get_wifi_sta_dns( void );
		char *			get_wifi_sta_gw( void );
		char *			get_wifi_sta_ip( void );
		aws_ip_mode		get_wifi_sta_ip_mode( void );
		char *			get_wifi_sta_password( void );
		char			*get_wifi_sta_ssid( void );
		uint8_t			get_wind_vane_model( void );
		const char *	get_wind_vane_model_str( void );
		bool 			load( bool );
		void			reset_parameter( const char * );
		bool			rollback( void );
		bool			save_runtime_configuration( JsonVariant & );
		bool 			update( JsonVariant &json );
		
	private:

		uint8_t 			anemometer_model;
		bool				close_dome_on_rain;
		uint16_t			config_port;
		bool				debug_mode;
		uint32_t			devices;
		bool				initialised;
		float				msas_calibration_offset;
		AWSNetworkConfig	network_config;
		// flawfinder: ignore
		char	  			pcb_version[8];
		aws_pwr_src			pwr_mode;
		uint16_t			rain_event_guard_time;
		char				*remote_server;
		char				*tzname;
		char				*url_path;
		uint8_t				wind_vane_model;


		bool	read_config( void );
		bool	read_file( const char *, JsonDocument & );
		bool 	read_hw_info_from_nvs( void );
		bool	set_parameter( JsonDocument &, const char *, char **, const char * );
		bool	verify_entries( JsonVariant & );
};

#endif
