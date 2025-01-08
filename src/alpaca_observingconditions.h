/*
	alpaca_observingconditions.h

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023-2024 F.Lesage

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
#ifndef _ALPACA_OBSERVINGCONDITIONS_H
#define _ALPACA_OBSERVINGCONDITIONS_H

#include "alpaca_device.h"

const alpaca_interface_version_t	OBSERVINGCONDITIONS_INTERFACE_VERSION	= 1;

// Attempt to convert RG-9 rain scale to ASCOM's ...
const std::array<float,8> rain_rate = {
	0.F,			// No rain
	0.1F,			// Rain drops
	1.0F,			// Very light
	2.0F,			// Light
	2.5F,			// Medium light
	5.0F,			// Medium
	10.0F,			// Heavy
	50.1F			// Violent
};

class alpaca_observingconditions : public alpaca_device
{
	private:
			etl::string<256>		message_str;

		void build_property_description_answer( char *, const char *, etl::string<128> & );
		void build_timesincelastupdate_answer( char *, const char *, etl::string<128> & );

	public:

			explicit alpaca_observingconditions( void );
		void set_connected( AsyncWebServerRequest *request, etl::string<128> & );
		void get_averageperiod( AsyncWebServerRequest *request, etl::string<128> & );
		void set_averageperiod( AsyncWebServerRequest *request, etl::string<128> & );
		void cloudcover( AsyncWebServerRequest *request, etl::string<128> & );
		void dewpoint( AsyncWebServerRequest *request, etl::string<128> & );
		void humidity( AsyncWebServerRequest *request, etl::string<128> & );
		void pressure( AsyncWebServerRequest *request, etl::string<128> & );
		void rainrate( AsyncWebServerRequest *request, etl::string<128> & );
		void skybrightness( AsyncWebServerRequest *request, etl::string<128> & );
		void skyquality( AsyncWebServerRequest *request, etl::string<128> & );
		void skytemperature( AsyncWebServerRequest *request, etl::string<128> & );
		void temperature( AsyncWebServerRequest *request, etl::string<128> & );
		void winddirection( AsyncWebServerRequest *request, etl::string<128> & );
		void windgust( AsyncWebServerRequest *request, etl::string<128> & );
		void windspeed( AsyncWebServerRequest *request, etl::string<128> & );
		void refresh( AsyncWebServerRequest *request, etl::string<128> & );
		void sensordescription( AsyncWebServerRequest *request, etl::string<128> & );
		void timesincelastupdate( AsyncWebServerRequest *request, etl::string<128> & );

};
#endif
