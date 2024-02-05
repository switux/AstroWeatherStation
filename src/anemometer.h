/*
  	anemometer.h

	(c) 2023 F.Lesage

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

#define SEND	HIGH
#define RECV	LOW

class Anemometer : public Device {

	public:

		static const std::array<std::string, 3> _anemometer_model;
		static const std::array<std::string, 3> _anemometer_description;
		
				Anemometer( void );
		bool			initialise( SoftwareSerial *, uint32_t, byte, bool );
		float			get_wind_gust( void );
		float			get_wind_speed( bool );

	private:

	   	byte			model;
	   	byte			wind_speed_index	= 0;
   		byte			wind_speeds_size	= 0;
  	 	float			wind_gust			= 0.F;
  	 	float			wind_speed			= 0.F;
  	 	std::vector<float>	wind_speeds;
   		uint16_t		bps					= 0;
		uint8_t			cmd[8];
		uint8_t			answer[7];
		uint32_t		polling_ms_interval	= 0;
		SoftwareSerial	*sensor_bus			= nullptr;

		void uint64_t_to_uint8_t_array( uint64_t cmd, uint8_t *cmd_array );
};

#endif
