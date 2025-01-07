/*
  	AWSNetwork.h

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
#ifndef _AWSNetwork_h
#define _AWSNetwork_h

#include <ESPping.h>

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

		bool eth_post_content( const char *, etl::string<128> &, const char * );
		bool wifi_post_content( const char *, etl::string<128> &, const char * );

	public:

					AWSNetwork( void );
		IPAddress	cidr_to_mask( byte cidr );
		bool 		connect_to_wifi( void );
		IPAddress	get_ip( aws_iface );
		IPAddress	get_gw( aws_iface );
		IPAddress 	get_subnet( aws_iface );

		uint8_t		*get_wifi_mac( void );	
		bool		initialise( AWSConfig *, bool );
		bool		initialise_ethernet( void );
		bool		initialise_wifi( void );
		byte		mask_to_cidr( uint32_t );
		bool		post_content( const char *, size_t, const char * );
		bool		start_hotspot( void );
		void 		webhook( const char * );


};

#endif
