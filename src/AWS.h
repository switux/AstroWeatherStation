#pragma once
#ifndef _AWS_H
#define _AWS_H

void OTA_callback( int, int );

class AstroWeatherStation {

	private:
		bool				debug_mode,
							config_mode,
							rain_event,
							solar_panel;
		char				*json_sensor_data;
		char				uptime[32];
		uint8_t				eth_mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED },
							wifi_mac[6];
                
		EthernetClient		*ethernet;
		SSLClient			*ssl_eth_client;
		IPAddress			ap_dns,
							ap_gw,
							ap_ip,
							ap_subnet,
							sta_dns,
							sta_gw,
							sta_ip,
							sta_subnet,
							eth_dns,
							eth_gw,
							eth_ip,
							eth_subnet;
		aws_wifi_mode_t		current_wifi_mode;
		aws_iface_t			current_pref_iface;
						
		AWSSensorManager 	sensor_manager;
		AWSConfig			*config;
		AWSWebServer 		*server;
		alpaca_server		*alpaca;

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
		void		post_content( const char *, const char * );
		void		print_config_string( const char *, ... );
		void		print_runtime_config( void );
		void		report_unavailable_sensors( void );
		void		send_alarm( const char *, const char * );
		void		send_rain_event_alarm( uint8_t );
		bool		setup_pref_iface( void );
		bool		shutdown_wifi( void );
		bool		start_config_server( void );
		bool		start_hotspot( void );
		bool		startup_sanity_check( void );
		bool		stop_hotspot( void );
		void		wakeup_reason_to_string( esp_sleep_wakeup_cause_t, char * );

	public:

		                AstroWeatherStation( void );
		void            check_ota_updates( void );
		sensor_data_t   *get_sensor_data( void );
		uint16_t        get_config_port( void );
        byte            get_eth_cidr_prefix( void );
        IPAddress       *get_eth_dns( void );
        IPAddress       *get_eth_gw( void );
		IPAddress       *get_eth_ip( void );
        char            *get_json_sensor_data( void );
        char            *get_json_string_config( void );
        char            *get_root_ca( void );
		char            *get_uptime( void );
        byte            get_sta_cidr_prefix( void );
		IPAddress       *get_sta_dns( void );
		IPAddress       *get_sta_gw( void );
        IPAddress       *get_sta_ip( void );
        byte            get_ap_cidr_prefix( void );
		IPAddress       *get_ap_dns( void );
		IPAddress       *get_ap_gw( void );
        IPAddress       *get_ap_ip( void );
		void            handle_rain_event( void );
		bool            initialise( void );
		bool            is_rain_event( void );
		bool            on_solar_panel();
		void            reboot( void );
		void            read_sensors( void );
		void            send_data( void );
		void            initialise_sensors( void );
		bool            update_config( JsonVariant & );
};

#endif
