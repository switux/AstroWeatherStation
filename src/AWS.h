/*
  	AWS.h

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
#ifndef _AWS_H
#define _AWS_H

#include "AWSWeb.h"
#include "AWSSensorManager.h"
#include "dome.h"
#include "alpaca.h"

constexpr size_t DATA_JSON_STRING_MAXLEN = 1024;

void OTA_callback( int, int );

class AWSNetwork {

	private:

		AWSConfig			*config;
		aws_iface_t			current_pref_iface;
		aws_wifi_mode_t		current_wifi_mode;
		bool				debug_mode;
		IPAddress			eth_dns;
		IPAddress			eth_gw;
		IPAddress			eth_ip;
		IPAddress			eth_subnet;
		EthernetClient		*ethernet;
		SSLClient			*ssl_eth_client;
		IPAddress			wifi_ap_dns;
		IPAddress			wifi_ap_gw;
		IPAddress			wifi_ap_ip;
		IPAddress			wifi_ap_subnet;
		uint8_t				wifi_mac[6];
		IPAddress			wifi_sta_dns;
		IPAddress			wifi_sta_gw;
		IPAddress			wifi_sta_ip;
		IPAddress			wifi_sta_subnet;

	public:

					AWSNetwork( void );
		IPAddress	cidr_to_mask( byte cidr );
		bool 		connect_to_wifi( void );
		bool		disconnect_from_wifi( void );
		byte		get_eth_cidr_prefix( void );
		IPAddress 	*get_eth_dns( void );
		IPAddress	*get_eth_gw( void );
		IPAddress	*get_eth_ip( void );
		byte		get_wifi_sta_cidr_prefix( void );
		IPAddress	*get_wifi_sta_dns( void );
		IPAddress	*get_wifi_sta_gw( void );
		IPAddress	*get_wifi_sta_ip( void );
		uint8_t		*get_wifi_mac( void );	
		bool		initialise( AWSConfig *, bool );
		bool		initialise_ethernet( void );
		bool		initialise_wifi( void );
		byte		mask_to_cidr( uint32_t );
		bool		post_content( const char *, const char * );
		bool		shutdown_wifi( void );
		bool		start_hotspot( void );
		bool		stop_hotspot( void );

};

class AstroWeatherStation {

	private:

		bool				debug_mode;
		bool				config_mode;
		bool				ntp_synced;
		bool				rain_event;
		bool				solar_panel;
		char				json_sensor_data[ DATA_JSON_STRING_MAXLEN ];
		char				*ota_board;
		char				*ota_device;
		char				*ota_config;
							// flawfinder: ignore
		char				uptime[32];

		TaskHandle_t		aws_periodic_task_handle;

		AWSNetwork			network;
		AWSSensorManager 	*sensor_manager;
		AWSConfig			*config;
		AWSWebServer 		*server;
		alpaca_server		*alpaca;
		AWSDome				*dome;
		I2C_SC16IS750		*sc16is750;

		void		check_rain_event_guard_time( void );
		IPAddress	cidr_to_mask( byte );
		bool		connect_to_wifi( void );
		bool		disconnect_from_wifi( void );
		void		display_banner( void );
		void		enter_config_mode( void );
		bool		initialise_ethernet( void );
		bool		initialise_network( void );
		bool		initialise_wifi( void );
		byte		mask_to_cidr( uint32_t );
		const char	*OTA_message( int );
		void		periodic_tasks( void * );
		bool		post_content( const char *, const char * );
		void		print_config_string( const char *, ... );
		void		print_runtime_config( void );
		void		send_backlog_data( void );
		void		send_rain_event_alarm( const char * );
		bool		shutdown_wifi( void );
		bool		start_config_server( void );
		bool		start_hotspot( void );
		bool		startup_sanity_check( void );
		bool		stop_hotspot( void );
		bool		store_unsent_data( char * );
		void		wakeup_reason_to_string( esp_sleep_wakeup_cause_t, char * );

	public:

						AstroWeatherStation( void );
		void            check_ota_updates( void );
		const char		*get_anemometer_sensorname( void );
		bool			get_debug_mode( void );
		AWSDome			*get_dome( void );
		sensor_data_t   *get_sensor_data( void );
		uint16_t        get_config_port( void );
        byte            get_eth_cidr_prefix( void );
        IPAddress       *get_eth_dns( void );
        IPAddress       *get_eth_gw( void );
		IPAddress       *get_eth_ip( void );
        char            *get_json_sensor_data( void );
        char            *get_json_string_config( void );
        bool			get_location_coordinates( double *, double * );
        char            *get_root_ca( void );
		char            *get_uptime( void );
        byte            get_wifi_sta_cidr_prefix( void );
		IPAddress       *get_wifi_sta_dns( void );
		IPAddress       *get_wifi_sta_gw( void );
        IPAddress       *get_wifi_sta_ip( void );
		const char		*get_wind_vane_sensorname( void );
		void            handle_rain_event( void );
		bool			has_gps( void );
		bool			has_rain_sensor( void );
		bool            initialise( void );
		bool			is_sensor_initialised( uint8_t );
		bool            is_rain_event( void );
		bool			issafe( void );
		bool			is_ntp_synced( void );
		bool            on_solar_panel();
		bool			poll_sensors( void );
		bool			rain_sensor_available( void );
		void            reboot( void );
		void            read_sensors( void );
		void			report_unavailable_sensors( void );
		void			send_alarm( const char *, const char * );
		void            send_data( void );
		bool			sync_time( void );
		void            initialise_sensors( void );
		bool            update_config( JsonVariant & );
};

#endif
