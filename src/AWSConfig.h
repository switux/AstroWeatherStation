/*	
  	AWSConfig.h
  	
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

#pragma once
#ifndef _AWSConfig_H
#define _AWSConfig_H

#include <ArduinoJson.h>

extern const char *_anemometer_model[3];
extern const char *_windvane_model[3];

typedef enum {

	wifi_ap,
	wifi_sta,
	eth

} aws_iface_t;

typedef enum {

	sta,
	ap,
	both

} aws_wifi_mode_t;

typedef enum {

	panel,
	pwr,
	poe

} aws_pwr_src_t;

typedef enum {

	dhcp,
	fixed
	
} aws_ip_mode_t;

class AWSConfig {

	public:

						AWSConfig( void );
		bool			can_rollback( void );
		aws_iface_t		get_alpaca_iface( void );
		uint8_t *		get_anemometer_cmd( void );
		uint16_t		get_anemometer_com_speed( void );
		uint8_t			get_anemometer_model( void );
		const char *	get_anemometer_model_str( void );
		char *			get_ap_ssid( void );
		bool			get_close_dome_on_rain( void );
		aws_iface_t		get_config_iface( void );
		uint16_t		get_config_port( void );
		char *			get_eth_dns( void );
		char *			get_eth_gw( void );
		char *			get_eth_ip( void );
		aws_ip_mode_t	get_eth_ip_mode( void );
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
		aws_iface_t		get_pref_iface( void );
		aws_pwr_src_t	get_pwr_mode( void );
		uint16_t 		get_rain_event_guard_time( void );
		char            *get_remote_server( void );
		char            *get_root_ca( void );
		char            *get_sta_dns( void );
		char            *get_sta_gw( void );
		char            *get_sta_ip( void );
		char            *get_sta_ssid( void );
		char            *get_tzname( void );
		char            *get_url_path( void );
		aws_wifi_mode_t get_wifi_mode( void );
		char *			get_wifi_ap_dns( void );
		char *			get_wifi_ap_gw( void );
		char *			get_wifi_ap_ip( void );
		char *			get_wifi_ap_password( void );
		char *			get_wifi_sta_dns( void );
		char *			get_wifi_sta_gw( void );
		char *			get_wifi_sta_ip( void );
		aws_ip_mode_t	get_wifi_sta_ip_mode( void );
		char *			get_wifi_sta_password( void );
		uint8_t *		get_wind_vane_cmd( void );
		uint16_t		get_wind_vane_com_speed( void );
		uint8_t			get_wind_vane_model( void );
		const char *	get_wind_vane_model_str( void );
		bool 			load( bool );
		void			reset_parameter( const char * );
		bool			rollback( void );
		bool			save_runtime_configuration( JsonVariant & );
		bool 			update( JsonVariant &json );
		
	private:

		aws_pwr_src_t	pwr_mode;
		aws_wifi_mode_t	wifi_mode;
		aws_iface_t		alpaca_iface,
						config_iface,
						pref_iface;
		aws_ip_mode_t	eth_ip_mode,
						wifi_sta_ip_mode;
		
		bool		close_dome_on_rain,
					debug_mode,
					has_ethernet,
					has_bme,
					has_dome,
					has_gps,
					has_mlx,
					has_rain_sensor,
					has_sc16is750,
					has_tsl,
					has_ws,
					has_wv,
					initialised;
					
		float		msas_calibration_offset;
		
		char  		pcb_version[8],
					*remote_server,
					*root_ca,
					*sta_ssid,
					*eth_dns,
					*eth_ip,
					*eth_gw,
					*wifi_sta_password,
					*wifi_sta_dns,
					*wifi_sta_ip,
					*wifi_sta_gw,
					*ap_ssid,
					*wifi_ap_dns,
					*wifi_ap_password,
					*wifi_ap_ip,
					*wifi_ap_gw,
					*tzname,
					*url_path;
					
		uint8_t 	anemometer_cmd[ 8 ],
					anemometer_model,
					wind_vane_cmd[ 8 ],
					wind_vane_model;
		uint16_t	anemometer_com_speed,
					config_port,
					rain_event_guard_time,
					wind_vane_com_speed;
				
		bool	read_config( void );
		bool	read_file( const char *, JsonDocument & );
		bool 	read_hw_info_from_nvs( void );
		bool	set_parameter( JsonDocument &, const char *, char **, const char * );
		bool	verify_entries( JsonVariant & );
		
};

#endif
