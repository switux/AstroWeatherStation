/*
  	AstroWeatherStation.h

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
#ifndef _AstroWeatherStation_H
#define _AstroWeatherStation_H

#include "config_server.h"
#include "sensor_manager.h"
#include "dome.h"
#include "AWSLookout.h"
#include "alpaca_server.h"

const byte LOW_BATTERY_COUNT_MIN = 5;
const byte LOW_BATTERY_COUNT_MAX = 10;

const unsigned short 	BAT_V_MAX		= 4200;		// in mV
const unsigned short	BAT_V_MIN		= 3000;		// in mV
const byte 				BAT_LEVEL_MIN	= 33;		// in %, corresponds to ~3.4V for a typical Li-ion battery
const unsigned short	VCC				= 3300;		// in mV
const unsigned int		V_DIV_R1		= 82000;	// voltage divider R1 in ohms
const unsigned int		V_DIV_R2		= 300000;	// voltage divider R2 in ohms
const unsigned short	ADC_MAX			= 4096;		// 12 bits resolution
const float				V_MAX_IN		= ( BAT_V_MAX*V_DIV_R2 )/( V_DIV_R1+V_DIV_R2 );	// in mV
const float				V_MIN_IN		= ( BAT_V_MIN*V_DIV_R2 )/( V_DIV_R1+V_DIV_R2 );	// in mV
const unsigned short	ADC_V_MAX		= ( V_MAX_IN*ADC_MAX / VCC );
const unsigned short	ADC_V_MIN		= ( V_MIN_IN*ADC_MAX / VCC );

extern const unsigned long DOME_DEVICE;
extern const unsigned long ETHERNET_DEVICE;
extern const unsigned long SC16IS750_DEVICE;

struct ota_setup_t {
	char	*board	= nullptr;
	char	*config	= nullptr;
	char	*device	= nullptr;
	
};

struct station_devices_t {

	Dome			dome;
	I2C_SC16IS750	sc16is750;
	AWSGPS			gps;

};

void OTA_callback( int, int );

class AWSNetwork {

	private:

		AWSConfig			*config;
		aws_iface			current_pref_iface;
		aws_wifi_mode		current_wifi_mode;
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
		uint8_t				wifi_mac[6];		// NOSONAR
		IPAddress			wifi_sta_dns;
		IPAddress			wifi_sta_gw;
		IPAddress			wifi_sta_ip;
		IPAddress			wifi_sta_subnet;

	public:

					AWSNetwork( void );
		IPAddress	cidr_to_mask( byte cidr );
		bool 		connect_to_wifi( void );
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
		bool		post_content( const char *, size_t, const char * );
		bool		start_hotspot( void );
		bool		stop_hotspot( void );

};

class AstroWeatherStation {

	private:

		alpaca_server		alpaca;
		TaskHandle_t		aws_periodic_task_handle;
		AWSConfig			config;
		bool				debug_mode	= false;
		etl::string<1024>	json_sensor_data;
		etl::string<128>	location;
		AWSNetwork			network;
		bool				ntp_synced	= false;
		ota_setup_t			ota_setup;
		bool				rain_event	= false;
		bool				request_dome_shutter_open = false;
		AWSSensorManager 	sensor_manager;
		AWSWebServer 		server;
		bool				solar_panel;
		station_data_t		station_data;
		station_devices_t	station_devices;
		etl::string<32>		unique_build_id;
		AWSLookout			lookout;

		void			check_rain_event_guard_time( uint16_t );
		IPAddress		cidr_to_mask( byte );
		bool			connect_to_wifi( void );
		void			compute_uptime( void );
		bool 			determine_boot_mode( void );
		void			display_banner( void );
		void			enter_config_mode( void );
		template<typename... Args>
		etl::string<96>	format_helper( const char *, Args... );
		bool			initialise_ethernet( void );
		void			initialise_GPS( void );
		bool			initialise_network( void );
		bool			initialise_wifi( void );
		byte			mask_to_cidr( uint32_t );
		const char		*OTA_message( int );
		void			periodic_tasks( void * );
		bool			post_content( const char *, const char * );
		template<typename... Args>
		void			print_config_string( const char *, Args... );
		void			print_runtime_config( void );
		void			read_battery_level( void );
		    void read_GPS( void );

		int				reformat_ca_root_line( std::array<char,97> &, int, int, int, const char * );
		void			send_backlog_data( void );
		void			send_rain_event_alarm( const char * );
		void			start_alpaca_server( void );
		bool			start_config_server( void );
		bool			start_hotspot( void );
		bool			startup_sanity_check( void );
		bool			stop_hotspot( void );
		bool			store_unsent_data( etl::string_view );
		void			wakeup_reason_to_string( esp_sleep_wakeup_cause_t, char * );

	public:

							AstroWeatherStation( void );
		void				check_ota_updates( void );
		etl::string_view	get_anemometer_sensorname( void );
		bool				get_debug_mode( void );
		Dome				*get_dome( void );
		sensor_data_t		*get_sensor_data( void );
		station_data_t		*get_station_data( void );
		uint16_t			get_config_port( void );
		byte				get_eth_cidr_prefix( void );
		IPAddress			*get_eth_dns( void );
		IPAddress			*get_eth_gw( void );
		IPAddress			*get_eth_ip( void );
		etl::string_view	get_json_sensor_data( void );
		etl::string_view	get_json_string_config( void );
		etl::string_view	get_location( void );
		bool				get_location_coordinates( float *, float * );
        etl::string_view	get_root_ca( void );
		time_t				get_timestamp( void );
		etl::string_view	get_unique_build_id( void );
		uint32_t			get_uptime( void );
		byte				get_wifi_sta_cidr_prefix( void );
		IPAddress			*get_wifi_sta_dns( void );
		IPAddress			*get_wifi_sta_gw( void );
		IPAddress			*get_wifi_sta_ip( void );
		etl::string_view	get_wind_vane_sensorname( void );
		void				handle_dome_shutter_is_closed( void );
		void				handle_dome_shutter_is_moving( void );
		void				handle_dome_shutter_is_opening( void );
		void				handle_rain_event( void );
		bool				has_gps( void );
		bool				has_rain_sensor( void );
		bool				initialise( void );
		void				initialise_sensors( void );
		bool				is_sensor_initialised( uint8_t );
		bool				is_rain_event( void );
		bool				issafe( void );
		bool				is_ntp_synced( void );
		bool				on_solar_panel();
		bool				poll_sensors( void );
		bool				rain_sensor_available( void );
		void				reboot( void );
		void				read_sensors( void );
		void				report_unavailable_sensors( void );
		void				send_alarm( const char *, const char * );
		void				send_data( void );
		bool				sync_time( bool );
		bool				update_config( JsonVariant & );
};

#endif
