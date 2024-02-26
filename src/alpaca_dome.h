/*
	alpaca_dome.h

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
#ifndef _ALPACA_DOME_H
#define _ALPACA_DOME_H

#include "alpaca_device.h"
#include "dome.h"

const alpaca_interface_version_t	DOME_INTERFACE_VERSION	= 2;

typedef struct alpaca_dome_t {

	dome_shutter_status_t	shutterstatus;

} alpaca_dome_t;

class alpaca_dome : public alpaca_device
{
	private:

		dome_shutter_status_t	dome_shutter_status	= dome_shutter_status_t::Open;
		etl::string<256>		message_str;

	public:

		explicit	alpaca_dome( void );
		void		abortslew( AsyncWebServerRequest *, const char * );
		void		cansetshutter( AsyncWebServerRequest *, const char * );
		void		closeshutter( AsyncWebServerRequest *, const char * );
		void		openshutter( AsyncWebServerRequest *, const char * );
		void		set_connected( AsyncWebServerRequest *, const char * );
		void		slaved( AsyncWebServerRequest *, const char * );
		void		shutterstatus( AsyncWebServerRequest *, const char * );
};

#endif
