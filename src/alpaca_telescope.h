/*
	alpaca_telescope.h

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
#ifndef _ALPACA_TELESCOPE_H
#define _ALPACA_TELESCOPE_H

#include <SiderealPlanets.h>
#include "alpaca_device.h"

const alpaca_interface_version_t	TELESCOPE_INTERFACE_VERSION	= 3;

class alpaca_telescope : public alpaca_device
{
	private:

			SiderealPlanets		astro_lib;
			float				forced_latitude		= -1;
			float				forced_longitude	= -1;
			float				forced_altitude		= -1;
			etl::string<256>	message_str;

	public:

		explicit alpaca_telescope( void );
		void axisrates( AsyncWebServerRequest *, etl::string<128> & );
		void canmoveaxis( AsyncWebServerRequest *, etl::string<128> & );
		void set_connected( AsyncWebServerRequest *, etl::string<128> & );
		void siderealtime( AsyncWebServerRequest *, etl::string<128> & );
		void siteelevation( AsyncWebServerRequest *, etl::string<128> & );
		void set_siteelevation( AsyncWebServerRequest *, etl::string<128> & );
		void sitelatitude( AsyncWebServerRequest *, etl::string<128> & );
		void set_sitelatitude( AsyncWebServerRequest *, etl::string<128> & );
		void sitelongitude( AsyncWebServerRequest *, etl::string<128> & );
		void set_sitelongitude( AsyncWebServerRequest *, etl::string<128> & );
		void trackingrates( AsyncWebServerRequest *, etl::string<128> & );
		void utcdate( AsyncWebServerRequest *, etl::string<128> & );
		void set_utcdate( AsyncWebServerRequest *, etl::string<128> & );
};

#endif
