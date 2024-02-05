/*	
  	Hydreon.h
  	
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
#ifndef _HYDREON_H
#define _HYDREON_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "gpio_config.h"

class Hydreon : public Device {

	private:

		static constexpr byte				HYDREON_PROBE_RETRIES	= 3;
		static constexpr byte				RAIN_SENSOR_OK			= 0;
		static constexpr byte 				RAIN_SENSOR_FAIL		= 127;
		static const std::array<float,8>	rain_rates;
		static const std::array<uint16_t,7> bps;

		uint8_t				intensity;
		uint8_t				reset_pin	= GPIO_RAIN_SENSOR_MCLR;
		SemaphoreHandle_t 	rg9_read_mutex;
		uint8_t				rx_pin		= GPIO_RAIN_SENSOR_RX;
		uint8_t				tx_pin		= GPIO_RAIN_SENSOR_TX;
		HardwareSerial		sensor		= HardwareSerial(1);
		etl::string<128>	str;
		char				status;

		bool			initialise( void );
		void			probe( uint16_t );
		byte			read_string( void );
		void			try_baudrates( void );

	public:

					Hydreon( void );
		bool 		initialise( HardwareSerial &, bool );
		byte		get_rain_intensity( void  );
		const char	*get_rain_intensity_str( void );
		float		get_rain_rate( void );
		const char	*reset_cause( void );
};

#endif
