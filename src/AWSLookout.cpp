/*
  	AWSLookout.cpp

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

#include <esp_task_wdt.h>
// Keep these two to get rid of compile time errors because of incompatibilities between libraries
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>

#include "common.h"
#include "SC16IS750.h"
#include "dome.h"
#include "config_manager.h"
#include "sensor_manager.h"
#include "AWSLookout.h"
#include "AstroWeatherStation.h"

extern AstroWeatherStation	station;

template <typename T>
bool AWSLookout::check_safe_rule( const char *name, lookout_rule_t<T> &rule, T value, time_t value_ts, time_t now, lookout_rule_t<T> &unsafe_rule )
{
	if ( !rule.active ) {

		unsafe_rule.ts = 0;
		return ( rule.satisfied = true );
	}

	if ( value > rule.max )
		return ( rule.satisfied = false );

	if ( !rule.ts )
		rule.ts = value_ts;

	if (( now - rule.ts ) >= rule.delay ) {

		if ( std::is_same<T, float>::value )
			Serial.printf( "[LOOKOUT   ] [INFO ] Safe rule '%s' satisfied: %f <= %f\n", name, value, rule.max );
		else if ( std::is_same<T, uint8_t>::value )
			Serial.printf( "[LOOKOUT   ] [INFO ] Safe rule '%s' satisfied: %d <= %d\n", name, value, rule.max );

		unsafe_rule.ts = 0;
		return ( rule.satisfied = true );

	}

	if ( std::is_same<T, float>::value )
		Serial.printf( "[LOOKOUT   ] [INFO ] Safe rule '%s' pending delay: %f >= %f (%ds < %ds).\n", name, value, rule.max, now - rule.ts, rule.delay );
	else if ( std::is_same<T, uint8_t>::value )
		Serial.printf( "[LOOKOUT   ] [INFO ] Safe rule '%s' pending delay: %d >= %d (%ds < %ds).\n", name, value, rule.max, now - rule.ts, rule.delay );

	return ( rule.satisfied = false );
}

template <typename T>
bool AWSLookout::check_unsafe_rule( const char *name, lookout_rule_t<T> &rule, bool sensor_available, T value, time_t value_ts, time_t now, lookout_rule_t<T> &safe_rule )
{
	if ( !rule.active )
		return ( rule.satisfied = false );

	if ( !sensor_available && rule.check_available ) {

		Serial.printf( "[LOOKOUT   ] [INFO ] Unsafe rule '%s' satisfied: sensor not available\n", name );
		safe_rule.ts = 0;
		return ( rule.satisfied = true );
	}
	
	if ( value < rule.max )
		return ( rule.satisfied = false );

	if ( !rule.ts )
		rule.ts = value_ts;
	
	if (( now - rule.ts ) >= rule.delay ) {

		if ( std::is_same<T, float>::value )
			Serial.printf( "[LOOKOUT   ] [INFO ] Unsafe rule '%s' satisfied: %f >= %f\n", name, value, rule.max );
		else if ( std::is_same<T, uint8_t>::value )
			Serial.printf( "[LOOKOUT   ] [INFO ] Unsafe rule '%s' satisfied: %d >= %d\n", name, value, rule.max );

		safe_rule.ts = 0;
		return ( rule.satisfied = true );

	}

	if ( std::is_same<T, float>::value )
		Serial.printf( "[LOOKOUT   ] [INFO ] Unsafe rule '%s' pending delay: %f >= %f (%ds < %ds).\n", name, value, rule.max, now - rule.ts, rule.delay );
	else if ( std::is_same<T, uint8_t>::value )
		Serial.printf( "[LOOKOUT   ] [INFO ] Unsafe rule '%s' pending delay: %d >= %d (%ds < %ds).\n", name, value, rule.max, now - rule.ts, rule.delay );

	return ( rule.satisfied = false );
}

void AWSLookout::check_rules( void )
{
	aws_device_t		available_sensors	= sensor_manager->get_available_sensors();
	bool				b;
	time_t				now					= station.get_timestamp();
	etl::string<140>	str;
	bool				tmp_is_safe			= true;
	bool				tmp_is_unsafe		= false;
	
	if ( rain_event ) {

		Serial.printf( "[LOOKOUT   ] [INFO ] Rain event\n" );
		if ( unsafe_rain_event.active ) {

			unsafe_rain_event.ts = sensor_manager->get_sensor_data()->timestamp;
			unsafe_rain_event.satisfied = true;
			Serial.printf( "[LOOKOUT   ] [INFO ] Rain monitor is active, closing dome shutter.\n");
			tmp_is_unsafe |= true;

		}
		rain_event = false;

	} else {
	
		unsafe_rain_event.satisfied = false;

		tmp_is_unsafe |= ( b = AWSLookout::check_unsafe_rule<float>( "Wind speed #1",
																		unsafe_wind_speed_1,
																		sensor_manager->sensor_is_available( aws_device_t::ANEMOMETER_SENSOR ),
																		sensor_manager->get_sensor_data()->weather.wind_speed,
																		sensor_manager->get_sensor_data()->timestamp,
																		now,
																		safe_wind_speed ));
		tmp_is_unsafe |= ( b = AWSLookout::check_unsafe_rule<float>( "Wind speed #2",
																		unsafe_wind_speed_2,
																		sensor_manager->sensor_is_available( aws_device_t::ANEMOMETER_SENSOR ),
																		sensor_manager->get_sensor_data()->weather.wind_speed,
																		sensor_manager->get_sensor_data()->timestamp,
																		now,
																		safe_wind_speed ));
		tmp_is_unsafe |= ( b = AWSLookout::check_unsafe_rule<uint8_t>( "Cloud coverage #1",
																		unsafe_cloud_coverage_1,
																		sensor_manager->sensor_is_available( aws_device_t::MLX_SENSOR ),
																		sensor_manager->get_sensor_data()->weather.cloud_coverage,
																		sensor_manager->get_sensor_data()->timestamp,
																		now,
																		safe_cloud_coverage_1 ));
		if ( b )
			safe_cloud_coverage_2.ts = 0;

		tmp_is_unsafe |= ( b = AWSLookout::check_unsafe_rule<uint8_t>( "Cloud coverage #2",
																		unsafe_cloud_coverage_2,
																		sensor_manager->sensor_is_available( aws_device_t::MLX_SENSOR ),
																		sensor_manager->get_sensor_data()->weather.cloud_coverage,
																		sensor_manager->get_sensor_data()->timestamp,
																		now,
																		safe_cloud_coverage_1 ));
		if ( b )
			safe_cloud_coverage_2.ts = 0;

		tmp_is_unsafe |= ( b = AWSLookout::check_unsafe_rule<uint8_t>( "Rain intensity",
																		unsafe_rain_intensity,
																		sensor_manager->sensor_is_available( aws_device_t::RAIN_SENSOR ),
																		sensor_manager->get_sensor_data()->weather.rain_intensity,
																		sensor_manager->get_sensor_data()->timestamp,
																		now,
																		safe_rain_intensity ));

		tmp_is_safe &= ( b = AWSLookout::check_safe_rule<float>( "Wind speed",
																	safe_wind_speed,
																	sensor_manager->get_sensor_data()->weather.wind_speed,
																	sensor_manager->get_sensor_data()->timestamp,
																	now,
																	unsafe_wind_speed_1 ));
		if ( b )
			unsafe_wind_speed_2.ts = 0;

		tmp_is_safe &= ( b = AWSLookout::check_safe_rule<uint8_t>( "Cloud coverage #1",
																	safe_cloud_coverage_1,
																	sensor_manager->get_sensor_data()->weather.cloud_coverage,
																	sensor_manager->get_sensor_data()->timestamp,
																	now,
																	unsafe_cloud_coverage_1 ));
		if ( b )
			unsafe_cloud_coverage_2.ts = 0;

		tmp_is_safe &= ( b = AWSLookout::check_safe_rule<uint8_t>( "Cloud coverage #2",
																	safe_cloud_coverage_2,
																	sensor_manager->get_sensor_data()->weather.cloud_coverage,
																	sensor_manager->get_sensor_data()->timestamp,
																	now,
																	unsafe_cloud_coverage_1 ));
		if ( b )
			unsafe_cloud_coverage_2.ts = 0;

		tmp_is_safe &= ( b = AWSLookout::check_safe_rule<uint8_t>( "Rain intensity",
																	safe_rain_intensity,
																	sensor_manager->get_sensor_data()->weather.rain_intensity,
																	sensor_manager->get_sensor_data()->timestamp,
																	now,
																	unsafe_rain_intensity ));
	}

	if ( decide_is_safe( tmp_is_unsafe, tmp_is_safe ))
		return;
	
	snprintf( str.data(), str.capacity(), "[LOOKOUT   ] [INFO ] Safe conditions are <%s> AND unsafe conditions are <%s>: conditions are <UNDECIDED>, rules must be fixed!\n", tmp_is_safe?"SATISFIED":"NOT SATISFIED", tmp_is_unsafe?"SATISFIED":"NOT SATISFIED" );
	Serial.printf( "%s", str.data() );
	station.send_alarm( "[LOOKOUT] Configuration is not consistent", str.data() );
}

bool AWSLookout::decide_is_safe( bool unsafe, bool safe )
{
	if ( is_safe && !unsafe ) {

		Serial.printf( "[LOOKOUT   ] [INFO ] Previous conditions were <SAFE> AND unsafe conditions are <NOT SATISFIED>: conditions are <SAFE>\n" );
		dome->open_shutter();
		return true;

	}

	if ( !safe || unsafe ) {

		is_safe = false;
		Serial.printf( "[LOOKOUT   ] [INFO ] Safe conditions are <NOT SATISFIED> OR unsafe conditions are <SATISFIED>: conditions are <UNSAFE>\n" );
		dome->close_shutter();
		return true;
	}

	if ( !unsafe && safe ) {

		is_safe = true;
		Serial.printf( "[LOOKOUT   ] [INFO ] Safe conditions are <SATISFIED> AND unsafe conditions are <NOT SATISFIED>: conditions are <SAFE>\n" );
		dome->open_shutter();
		return true;
	}
}

etl::string_view AWSLookout::get_rules_state( void )
{
	DynamicJsonDocument	rules_state_json(140);
	int					i;
	
	rules_state_json["unsafe_rain_event"] = unsafe_rain_event.satisfied;
	rules_state_json["unsafe_wind_speed_1"] = unsafe_wind_speed_1.satisfied;
	rules_state_json["unsafe_wind_speed_2"] = unsafe_wind_speed_2.satisfied;
	rules_state_json["unsafe_cloud_coverage_1"] = unsafe_cloud_coverage_1.satisfied;
	rules_state_json["unsafe_cloud_coverage_2"] = unsafe_cloud_coverage_2.satisfied;
	rules_state_json["unsafe_rain_intensity"] = unsafe_rain_intensity.satisfied;

	rules_state_json["safe_wind_speed"] = safe_wind_speed.satisfied;
	rules_state_json["safe_cloud_coverage_1"] = safe_cloud_coverage_1.satisfied;
	rules_state_json["safe_cloud_coverage_2"] = safe_cloud_coverage_2.satisfied;
	rules_state_json["safe_rain_intensity"] = safe_rain_intensity.satisfied;

	if ( (i = measureJson( rules_state_json )) > rules_state_data.capacity() ) {

		etl::string<64> tmp;
		Serial.printf( "[LOOKOUT   ] [BUG  ] rules_state_data json is too small (%d > %d). Please report to support!\n", i, rules_state_data.capacity() );
		snprintf( tmp.data(), tmp.capacity(), "rules_state_data json is too small (%d > %d)", i, rules_state_data.capacity() );
		station.send_alarm( "[STATION] LOOKOUT BUG", tmp.data() );
		return etl::string_view( "" );

	}
	int l = serializeJson( rules_state_json, rules_state_data.data(), rules_state_data.capacity() );
	if ( debug_mode )
		Serial.printf( "[LOOKOUT   ] [DEBUG] lookout_data is %d bytes long, max size is %d bytes.\n", l, rules_state_data.capacity() );

	return etl::string_view( rules_state_data );
}

void AWSLookout::loop( void * )	// NOSONAR
{
	while( true ) {

		if ( initialised )
			check_rules();
		delay( 1000 );

	}
}

void AWSLookout::initialise( AWSConfig *_config, AWSSensorManager *_mngr, Dome *_dome, bool _debug_mode )
{
	debug_mode = _debug_mode;
	dome = _dome;
	sensor_manager = _mngr;

	Serial.printf( "[LOOKOUT   ] [INFO ] Initialising.\n" );

	initialise_rules( _config );

	std::function<void(void *)> _loop = std::bind( &AWSLookout::loop, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
			std::function<void(void*)>* periodic_tasks_proxy = static_cast<std::function<void(void*)>*>( param );	// NOSONAR
			(*periodic_tasks_proxy)( NULL );
		}, "AWSLookout Task", 10000, &_loop, 5, &watcher_task_handle, 1 );

	initialised = true;
	Serial.printf( "[LOOKOUT   ] [INFO ] Initialised.\n" );
}

void AWSLookout::initialise_rules( AWSConfig *_config )
{
	unsafe_rain_event.active = true;
	unsafe_rain_event.max = 1;
	unsafe_rain_event.delay = 0;
	unsafe_rain_event.check_available = 1;
	unsafe_rain_event.ts = 0;
	unsafe_rain_event.satisfied = false;
	
	unsafe_wind_speed_1.active = _config->get_parameter<bool>( "unsafe_wind_speed_1_active" ) && _config->get_has_device( aws_device_t::ANEMOMETER_SENSOR );
	unsafe_wind_speed_1.max = _config->get_parameter<float>( "unsafe_wind_speed_1_max" );
	unsafe_wind_speed_1.delay = _config->get_parameter<int>( "unsafe_wind_speed_1_delay" );
	unsafe_wind_speed_1.check_available = _config->get_parameter<int>( "unsafe_wind_speed_1_missing" );
	unsafe_wind_speed_1.ts = 0;
	
	unsafe_wind_speed_2.active = _config->get_parameter<bool>( "unsafe_wind_speed_2_active" ) && _config->get_has_device( aws_device_t::ANEMOMETER_SENSOR );
	unsafe_wind_speed_2.max =  _config->get_parameter<float>( "unsafe_wind_speed_2_max" );
	unsafe_wind_speed_2.delay =  _config->get_parameter<int>( "unsafe_wind_speed_2_delay" );
	unsafe_wind_speed_2.check_available = _config->get_parameter<int>( "unsafe_wind_speed_2_missing" );
	unsafe_wind_speed_2.ts = 0;

	unsafe_cloud_coverage_1.active = _config->get_parameter<bool>( "unsafe_cloud_coverage_1_active" ) && _config->get_has_device( aws_device_t::MLX_SENSOR );
	unsafe_cloud_coverage_1.max = _config->get_parameter<int>( "unsafe_cloud_coverage_1_max" );
	unsafe_cloud_coverage_1.delay = _config->get_parameter<int>( "unsafe_cloud_coverage_1_delay" );
	unsafe_cloud_coverage_1.check_available = _config->get_parameter<int>( "unsafe_cloud_coverage_1_missing" );
	unsafe_cloud_coverage_1.ts = 0;
	
	unsafe_cloud_coverage_2.active = _config->get_parameter<bool>( "unsafe_cloud_coverage_2_active" ) && _config->get_has_device( aws_device_t::MLX_SENSOR );
	unsafe_cloud_coverage_2.max = _config->get_parameter<int>( "unsafe_cloud_coverage_2_max" );
	unsafe_cloud_coverage_2.delay = _config->get_parameter<int>( "unsafe_cloud_coverage_2_delay" );
	unsafe_cloud_coverage_2.check_available = _config->get_parameter<int>( "unsafe_cloud_coverage_2_missing" );
	unsafe_cloud_coverage_2.ts = 0;

	unsafe_rain_intensity.active = _config->get_parameter<bool>( "unsafe_rain_intensity_active" ) && _config->get_has_device( aws_device_t::RAIN_SENSOR );
	unsafe_rain_intensity.max = _config->get_parameter<int>( "unsafe_rain_intensity_max" );
	unsafe_rain_intensity.delay = 0;
	unsafe_rain_intensity.check_available = _config->get_parameter<int>( "unsafe_rain_intensity_missing" );
	unsafe_rain_intensity.ts = 0;

	safe_wind_speed.active = _config->get_parameter<bool>( "safe_wind_speed_active" ) && _config->get_has_device( aws_device_t::ANEMOMETER_SENSOR );
	safe_wind_speed.max = _config->get_parameter<float>( "safe_wind_speed_max" );
	safe_wind_speed.delay = _config->get_parameter<int>( "safe_wind_speed_delay" );
	safe_wind_speed.check_available = false;
	safe_wind_speed.ts = 0;

	safe_cloud_coverage_1.active = _config->get_parameter<bool>( "safe_cloud_coverage_1_active" ) && _config->get_has_device( aws_device_t::MLX_SENSOR );
	safe_cloud_coverage_1.max = _config->get_parameter<int>( "safe_cloud_coverage_1_max" );
	safe_cloud_coverage_1.delay = _config->get_parameter<int>( "safe_cloud_coverage_1_delay" );
	safe_cloud_coverage_1.check_available = false;
	safe_cloud_coverage_1.ts = 0;
	
	safe_cloud_coverage_2.active = _config->get_parameter<bool>( "safe_cloud_coverage_2_active" ) && _config->get_has_device( aws_device_t::MLX_SENSOR );
	safe_cloud_coverage_2.max = _config->get_parameter<int>( "safe_cloud_coverage_2_max" );
	safe_cloud_coverage_2.delay = _config->get_parameter<int>( "safe_cloud_coverage_2_delay" );
	safe_cloud_coverage_2.check_available = false;
	safe_cloud_coverage_2.ts = 0;

	safe_rain_intensity.active = _config->get_parameter<bool>( "safe_rain_intensity_active" ) && _config->get_has_device( aws_device_t::RAIN_SENSOR );
	safe_rain_intensity.max = _config->get_parameter<int>( "safe_rain_intensity_max" );
	safe_rain_intensity.delay = _config->get_parameter<int>( "safe_rain_intensity_delay" );
	safe_rain_intensity.check_available = false;
	safe_rain_intensity.ts = 0;
}

bool AWSLookout::issafe( void )
{
	return is_safe;
}

void AWSLookout::resume( void )
{
	if ( initialised )
		vTaskResume( watcher_task_handle );
}

void AWSLookout::set_rain_event()
{
	rain_event = true;
}

void AWSLookout::suspend( void )
{
	if ( initialised )
		vTaskSuspend( watcher_task_handle );
}
