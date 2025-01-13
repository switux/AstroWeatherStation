/*
  	anemometer.h

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
#ifndef _anemometer_H
#define	_anemometer_H

#include <SoftwareSerial.h>
#include <vector>

#include "rs485_device.h"

class Anemometer : public RS485Device {

	public:

		static const std::array<std::string, 3> ANEMOMETER_MODEL;
		static const std::array<std::string, 3> ANEMOMETER_DESCRIPTION;
		static const std::array<uint64_t,3>		ANEMOMETER_CMD;
		static const std::array<uint16_t,3>		ANEMOMETER_SPEED;

				Anemometer( void ) = default;
		bool			initialise( SoftwareSerial *, uint32_t, byte, bool );
		float			get_wind_gust( void );
		float			get_wind_speed( bool );

	private:

	   	byte					model;
		uint32_t				polling_ms_interval	= 0;
	   	byte					wind_speed_index	= 0;
   		byte					wind_speeds_size	= 0;
  	 	float					wind_gust			= 0.F;
  	 	float					wind_speed			= 0.F;
  	 	std::vector<float>		wind_speeds;
};

#endif
