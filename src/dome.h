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

class AWSDome {

	private:

		bool				close_dome,
							is_connected,
							debug_mode;
		I2C_SC16IS750		*sc16is750;
		SemaphoreHandle_t	i2c_mutex;
		TaskHandle_t		dome_task_handle;

	public:

		explicit AWSDome( bool );
		AWSDome( I2C_SC16IS750 *, SemaphoreHandle_t, bool );
		void close( void * );
		bool closed( void );
		bool get_connected( void );
		void start_control_task( void );
		void trigger_close( void );
};

#endif
