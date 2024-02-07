/*
	alpaca_safetymonitor.h

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
#ifndef _ALPACA_SAFETYMONITOR_H
#define _ALPACA_SAFETYMONITOR_H

#define	SAFETYMONITOR_SUPPORTED_ACTIONS		"[]"
#define SAFETYMONITOR_DRIVER_INFO			"OpenAstroDevices driver for Hydreon RG-9 / v1.0.0"
#define SAFETYMONITOR_DESCRIPTION			"OpenAstroDevices Safety Monitor based on Hydreon RG-9 / v1.0.0"
#define SAFETYMONITOR_NAME					"OpenAstroDevices driver for Hydreon RG-9 / v1.0.0"
#define SAFETYMONITOR_DRIVER_VERSION		"1.0"
#define SAFETYMONITOR_INTERFACE_VERSION		1

#include "alpaca_device.h"

class alpaca_safetymonitor : public alpaca_device
{
	private:
			etl::string<256>		message_str;
	public:

		explicit alpaca_safetymonitor( void );

		void issafe( AsyncWebServerRequest *, const char * );
		void set_connected( AsyncWebServerRequest *, const char * );
};

#endif
