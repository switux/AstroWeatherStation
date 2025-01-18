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

#include "AWSOTA.h"
#include "AWSUpdater.h"
#include "config_server.h"
#include "sensor_manager.h"
#include "dome.h"
#include "AWSLookout.h"
#include "alpaca_server.h"
#include "AWSNetwork.h"

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

enum struct aws_ip_info : uint8_t
{
	ETH_DNS,
	ETH_GW,
	ETH_IP,
	WIFI_STA_DNS,
	WIFI_STA_GW,
	WIFI_STA_IP
};
using aws_ip_info_t = aws_ip_info;

enum struct aws_events : uint8_t
{
	RAIN,
	DOME_SHUTTER_CLOSED_CHANGE,
	DOME_SHUTTER_OPEN_CHANGE

};
using aws_event_t = aws_events;

enum struct boot_mode : uint8_t
{
	NORMAL,
	MAINTENANCE,
	FACTORY_RESET	
};
using aws_boot_mode_t = boot_mode;

enum struct station_status :uint8_t
{
	READY,
	BOOTING,
	CONFIG_ERROR,
	NETWORK_INIT,
	NETWORK_ERROR,
	SENSOR_INIT_ERROR,
	OTA_UPGRADE
};
using station_status_t = station_status;

struct ota_setup_t {

	etl::string<24>	board;
	etl::string<32>	config;
	etl::string<18>	device;
	etl::string<26>	version;
	ota_status_t	status_code		= ota_status_t::UNKNOWN;
	int32_t			status_ts		= 0;
	int32_t			last_update_ts	= 0;
};

struct station_devices_t {

	Dome			dome;
	I2C_SC16IS750	sc16is750;
	AWSGPS			gps;

};

enum class aws_operation_info_t : unsigned int {
	NONE				= 0x0000,
	FORCE_OTA_UPDATE	= 0x0001,
	NTP_SYNCED			= 0x0002,
	RAIN				= 0x0004,
	REQ_DOME_OPEN		= 0x0008,
	DEBUG				= 0x4000,
	READY				= 0x8000
};

void OTA_callback( int, int );

class AstroWeatherStation {

	private:

		alpaca_server				alpaca;
		TaskHandle_t				aws_led_task_handle;
		TaskHandle_t				aws_periodic_task_handle;
		AWSConfig					config;
		aws_operation_info_t		operation_info				= aws_operation_info_t::NONE;
		etl::string<1116>			json_sensor_data;
		station_status_t			led_status					= station_status_t::BOOTING;
		etl::string<128>			location;
		AWSLookout					lookout;
		AWSNetwork					network;
		AWSOTA						ota;
		ota_setup_t					ota_setup;
		AWSSensorManager 			sensor_manager;
		AWSWebServer 				server;
		bool						solar_panel;
		station_data_t				station_data;
		station_devices_t			station_devices;
		AWSUpdater					updater;

		void			check_rain_event_guard_time( uint16_t );
		void			compute_uptime( void );
		aws_boot_mode_t	determine_boot_mode( void );
		void			display_banner( void );
		void			check_factory_reset( aws_boot_mode_t );
		template<typename... Args>
		etl::string<96>	format_helper( const char *, Args... );
		void 			initialise_dome( void );
		void			initialise_GPS( void );
		void			led_task( void * );
		const char		*OTA_message( ota_status_t );
		void			periodic_tasks( void * );
		template<typename... Args>
		void			print_config_string( const char *, Args... );
		void			print_runtime_config( void );
		void			read_battery_level( void );
		void			read_GPS( void );
		int				reformat_ca_root_line( std::array<char,97> &, int, int, int, const char * );
		void			send_backlog_data( void );
		void			send_rain_event_alarm( const char * );
		void			set_led_status( station_status );
		void			start_alpaca_server( void );
		bool			start_config_server( void );
		bool			startup_sanity_check( void );
		bool			store_unsent_data( etl::string_view, size_t );
		void			try_enter_config_mode( aws_boot_mode_t );

	public:

							AstroWeatherStation( void );
		void				check_ota_updates( bool );
		void				close_dome_shutter( void );
		etl::string_view	get_anemometer_sensorname( void );
		Dome				*get_dome( void );
		sensor_data_t		*get_sensor_data( void );
		station_data_t		*get_station_data( void );
		uint16_t			get_config_port( void );
		etl::string_view	get_json_sensor_data( size_t * );
		etl::string_view	get_json_string_config( void );
		etl::string_view	get_json_string_run_config( void );
		etl::string_view	get_location( void );
		bool				get_location_coordinates( float *, float * );
		etl::string_view	get_lookout_rules_state_json_string( void );
        etl::string_view	get_root_ca( void );
		time_t				get_timestamp( void );
		etl::string_view	get_unique_build_id( void );
		uint32_t			get_uptime( void );
		etl::string_view	get_wind_vane_sensorname( void );
		void				handle_event( aws_event_t );
		bool				has_device( aws_device_t );
		bool				initialise( void );
		void				initialise_sensors( void );
		bool				is_rain_event( void );
		bool				is_ready( void );
		bool				is_sensor_initialised( aws_device_t );
		bool				issafe( void );
		bool				is_ntp_synced( void );
		bool				on_solar_panel();
		void				open_dome_shutter( void );
		bool				poll_sensors( void );
		bool				rain_sensor_available( void );
		void				reboot( void );
		void				read_sensors( void );
		void				report_unavailable_sensors( void );
		bool				resume_lookout( void );
		void				send_alarm( const char *, const char * );
		void				send_data( void );
		bool				suspend_lookout( void );
		bool				sync_time( bool );
		void				trigger_ota_update( void );
		bool				update_config( JsonVariant & );
};

#endif
