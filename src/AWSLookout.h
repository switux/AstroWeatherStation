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

template <typename T>
struct lookout_rule_t {
	bool	active;
	bool	check_available;
	T 		max;
	int 	delay;
	time_t	ts;
	bool	satisfied;
};

class AWSLookout
{
	private:

		TaskHandle_t			watcher_task_handle;
		bool					active					= false;
		bool 					debug_mode				= false;
		Dome					*dome					= nullptr;
		bool					initialised				= false;
		bool					is_safe					= true;
		etl::string<248>		rules_state_data;
		bool					rain_event				= false;
		AWSSensorManager		*sensor_manager			= nullptr;
		AWSConfig				*config					= nullptr;
		lookout_rule_t<uint8_t>	unsafe_rain_event;
		lookout_rule_t<float>	unsafe_wind_speed_1;
		lookout_rule_t<float>	unsafe_wind_speed_2;
		lookout_rule_t<uint8_t>	unsafe_cloud_coverage_1;
		lookout_rule_t<uint8_t>	unsafe_cloud_coverage_2;
		lookout_rule_t<uint8_t>	unsafe_rain_intensity;
		lookout_rule_t<float>	safe_wind_speed;
		lookout_rule_t<uint8_t>	safe_cloud_coverage_1;
		lookout_rule_t<uint8_t>	safe_cloud_coverage_2;
		lookout_rule_t<uint8_t>	safe_rain_intensity;

		void check_rules( void );
		template <typename T>
		bool check_unsafe_rule( const char *, lookout_rule_t<T> &, bool, T, time_t, time_t, lookout_rule_t<T> & );
		template <typename T>
		bool check_safe_rule( const char *, lookout_rule_t<T> &, T, time_t, time_t, lookout_rule_t<T> & );
		bool decide_is_safe( bool, bool );
		void initialise_rules( AWSConfig * );
		
	public:

							AWSLookout( void ) = default;
		etl::string_view	get_rules_state( void );
		void				initialise( AWSConfig *, AWSSensorManager *, Dome *, bool _debug_mode );
		bool				is_active( void );
		bool				issafe( void );
		void				loop( void * );
		void				set_rain_event( void );
		bool				suspend( void );
		bool				resume( void );
};

#endif
