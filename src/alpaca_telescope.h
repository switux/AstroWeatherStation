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

const alpaca_interface_version_t	TELESCOPE_INTERFACE_VERSION	= 3;

#include <SiderealPlanets.h>
#include "alpaca_device.h"

class alpaca_telescope : public alpaca_device
{
	private:

			SiderealPlanets		astro_lib;
			double				forced_latitude		= -1;
			double				forced_longitude	= -1;
			double				forced_altitude		= -1;
			etl::string<256>	message_str;

	public:

		explicit alpaca_telescope( void );
		void set_connected( AsyncWebServerRequest *, const char * );
		void siderealtime( AsyncWebServerRequest *, const char * );
		void siteelevation( AsyncWebServerRequest *, const char * );
		void set_siteelevation( AsyncWebServerRequest *, const char * );
		void sitelatitude( AsyncWebServerRequest *, const char * );
		void set_sitelatitude( AsyncWebServerRequest *, const char * );
		void sitelongitude( AsyncWebServerRequest *, const char * );
		void set_sitelongitude( AsyncWebServerRequest *, const char * );
		void trackingrates( AsyncWebServerRequest *, const char * );
		void utcdate( AsyncWebServerRequest *, const char * );
		void set_utcdate( AsyncWebServerRequest *, const char * );
		void axisrates( AsyncWebServerRequest *, const char * );
};

#endif
