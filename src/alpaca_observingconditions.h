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

const alpaca_interface_version_t	OBSERVINGCONDITIONS_INTERFACE_VERSION	= 1;

#include "alpaca_device.h"

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

		void build_property_description_answer( char *, const char *, const char * );
		void build_timesincelastupdate_answer( char *, const char *, const char * );

	public:

			explicit alpaca_observingconditions( void );
		void set_connected( AsyncWebServerRequest *request, const char * );
		void get_averageperiod( AsyncWebServerRequest *request, const char * );
		void set_averageperiod( AsyncWebServerRequest *request, const char * );
		void cloudcover( AsyncWebServerRequest *request, const char * );
		void dewpoint( AsyncWebServerRequest *request, const char * );
		void humidity( AsyncWebServerRequest *request, const char * );
		void pressure( AsyncWebServerRequest *request, const char * );
		void rainrate( AsyncWebServerRequest *request, const char * );
		void skybrightness( AsyncWebServerRequest *request, const char * );
		void skyquality( AsyncWebServerRequest *request, const char * );
		void skytemperature( AsyncWebServerRequest *request, const char * );
		void temperature( AsyncWebServerRequest *request, const char * );
		void winddirection( AsyncWebServerRequest *request, const char * );
		void windgust( AsyncWebServerRequest *request, const char * );
		void windspeed( AsyncWebServerRequest *request, const char * );
		void refresh( AsyncWebServerRequest *request, const char * );
		void sensordescription( AsyncWebServerRequest *request, const char * );
		void timesincelastupdate( AsyncWebServerRequest *request, const char * );

};
#endif
