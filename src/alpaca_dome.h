/*
	alpaca_dome.h

	ASCOM ALPACA Server for the AstroWeatherStation (c) 2023 F.Lesage

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

#define	DOME_SUPPORTED_ACTIONS	"[]"
#define DOME_DRIVER_INFO		"OpenAstroDevices driver for relay driven roll-off roof / v1.0.0"
#define	DOME_NAME				"OpenAstroDevices relay driven roll-off roof / v1.0.0"
#define	DOME_DESCRIPTION		"OpenAstroDevices driver for relay driven roll-off roof / v1.0.0"
#define DOME_DRIVER_VERSION		"1.0"
#define	DOME_INTERFACE_VERSION	2

#include "alpaca_device.h"

enum struct dome_shutter_status_type : byte
{
	Open,
	Closed,
	Opening,
	Closing,
	Error
};

using dome_shutter_status_t = dome_shutter_status_type;

typedef struct alpaca_dome_t {

	dome_shutter_status_t	shutterstatus;

} alpaca_dome_t;

class alpaca_dome : public alpaca_device
{
	private:

		dome_shutter_status_t	dome_shutter_status;

	public:

		explicit	alpaca_dome( bool );
		void		abortslew( AsyncWebServerRequest *, const char * );
		void		cansetshutter( AsyncWebServerRequest *, const char * );
		void		closeshutter( AsyncWebServerRequest *, const char * );
		void		openshutter( AsyncWebServerRequest *, const char * );
		void		set_connected( AsyncWebServerRequest *, const char * );
		void		slaved( AsyncWebServerRequest *, const char * );
		void		shutterstatus( AsyncWebServerRequest *, const char * );
};

#endif
