/*
  	AWSLookout.h

	(c) 2024 F.Lesage

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
#ifndef _AWSLookout_H
#define _AWSLookout_H

class AWSLookout
{
	private:

		TaskHandle_t		watcher_task_handle;
		bool 				debug_mode;
		Dome				*dome;
		AWSSensorManager	*sensor_manager;
		AWSConfig			*config;

	public:

		AWSLookout( void ) = default;
		void initialise( AWSConfig *, AWSSensorManager *, Dome *, bool _debug_mode );
		void loop( void * );
};

#endif
