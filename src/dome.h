/*	
  	dome.h
  	
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
#ifndef _DOME_H
#define	_DOME_H

#include "device.h"

void IRAM_ATTR _handle_dome_shutter_is_moving( void );

enum struct dome_shutter_status_type : byte
{
	Open,
	Closed,
	Opening,
	Closing,
	Error
};
using dome_shutter_status_t = dome_shutter_status_type;

class Dome : public Device {

	private:

		bool					catch_shutter_moving			= false;
		bool					do_close_shutter				= false;
		bool					is_connected					= false;
		bool					debug_mode						= false;
		I2C_SC16IS750			*sc16is750						= nullptr;
		SemaphoreHandle_t		i2c_mutex;
		bool					do_open_shutter					= false;
		TaskHandle_t			dome_task_handle;
		bool					_shutter_is_moving				= false;
		dome_shutter_status_t	shutter_status					= dome_shutter_status_t::Error;
		time_t					shutter_moving_ts				= 0;
		uint16_t				dome_shutter_moving_guard_time	= 0;

	public:

		explicit Dome( void );
		void					check_guard_times( void );
		void					close_shutter( void );
		void					control_task( void * );
		bool					get_connected( void );
		bool					get_shutter_closed_status( void );
		void					initialise( uint16_t, bool );
		void					initialise( I2C_SC16IS750 *, SemaphoreHandle_t, uint16_t, bool );
		void					open_shutter( void );
		void 					shutter_is_moving( void );
		void					trigger_close_shutter( void );
};

#endif
