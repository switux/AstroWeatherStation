#include <Arduino.h>
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

void AWSLookout::check_rules( void )
{
	bool	tmp_is_safe		= true;
	bool	tmp_is_unsafe	= false;
	time_t	now = station.get_timestamp();
	
	if ( rain_event ) {
		Serial.printf( "[INFO] Lookout: rain event\n" );
		if ( unsafe_rain_intensity.active ) {

			unsafe_rain_intensity.ts = sensor_manager->get_sensor_data()->timestamp;
			Serial.printf( "UNSAFE RAIN EVENT\n");
			tmp_is_unsafe |= true;
		}
		rain_event = false;
	} else
	

	if ( unsafe_wind_speed_1.active ) {

		if ( sensor_manager->get_sensor_data()->wind_speed >= unsafe_wind_speed_1.max ) {

			if ( !unsafe_wind_speed_1.ts )
				 unsafe_wind_speed_1.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - unsafe_wind_speed_1.ts ) >= unsafe_wind_speed_1.delay ) {
					Serial.printf( "UNSAFE WS1: %f >= %f\n", sensor_manager->get_sensor_data()->wind_speed,unsafe_wind_speed_1.max );
					tmp_is_unsafe |= true;
				} else
					Serial.printf( "UNSAFE WS1: %f >= %f but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->wind_speed, unsafe_wind_speed_1.max, now - unsafe_wind_speed_1.ts,  unsafe_wind_speed_1.delay);

			}
		}
	}

	if ( unsafe_wind_speed_2.active ) {

		if ( sensor_manager->get_sensor_data()->wind_speed >= unsafe_wind_speed_2.max ) {

			if ( !unsafe_wind_speed_2.ts )
				 unsafe_wind_speed_2.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - unsafe_wind_speed_2.ts ) >= unsafe_wind_speed_2.delay ) {
					Serial.printf( "UNSAFE WS2: %f >= %f\n", sensor_manager->get_sensor_data()->wind_speed,unsafe_wind_speed_2.max );
					tmp_is_unsafe |= true;
				} else
					Serial.printf( "UNSAFE WS2: %f >= %f but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->wind_speed, unsafe_wind_speed_2.max, now - unsafe_wind_speed_2.ts,  unsafe_wind_speed_2.delay);

			}
		}
	}

	if ( unsafe_cloud_coverage_1.active ) {

		if ( sensor_manager->get_sensor_data()->cloud_coverage >= unsafe_cloud_coverage_1.max ) {

			if ( !unsafe_cloud_coverage_1.ts )
				 unsafe_cloud_coverage_1.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - unsafe_cloud_coverage_1.ts ) >= unsafe_cloud_coverage_1.delay ) {
					Serial.printf( "UNSAFE CC1: %d >= %d\n", sensor_manager->get_sensor_data()->cloud_coverage,unsafe_cloud_coverage_1.max );
					tmp_is_unsafe |= true;
				} else {
					Serial.printf( "UNSAFE CC2: %d >= %d but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->cloud_coverage, unsafe_cloud_coverage_1.max, now - unsafe_cloud_coverage_1.ts, unsafe_cloud_coverage_1.delay  );
				
				}
			}
		}
	}

	if ( unsafe_cloud_coverage_2.active ) {

		if ( sensor_manager->get_sensor_data()->cloud_coverage >= unsafe_cloud_coverage_2.max ) {

			if ( !unsafe_cloud_coverage_2.ts )
				 unsafe_cloud_coverage_2.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - unsafe_cloud_coverage_2.ts ) >= unsafe_cloud_coverage_2.delay ) {
					Serial.printf( "UNSAFE CC2: %d >= %d\n", sensor_manager->get_sensor_data()->cloud_coverage,unsafe_cloud_coverage_2.max );
					tmp_is_unsafe |= true;
				} else
					Serial.printf( "UNSAFE CC2: %d >= %d but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->cloud_coverage, unsafe_cloud_coverage_2.max, now - unsafe_cloud_coverage_2.ts,unsafe_cloud_coverage_2.delay  );
				
			}
		}
	}
	
	if ( unsafe_rain_intensity.active ) {

		if ( sensor_manager->get_sensor_data()->rain_intensity > unsafe_rain_intensity.max ) {

			tmp_is_unsafe |= true;
			Serial.printf( "UNSAFE RI: %d > %d\n", sensor_manager->get_sensor_data()->rain_intensity, unsafe_rain_intensity.max );
		}
	}

	if ( safe_wind_speed.active ) {

		if ( sensor_manager->get_sensor_data()->wind_speed <= safe_wind_speed.max ) {

			if ( !safe_wind_speed.ts )
				 safe_wind_speed.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - safe_wind_speed.ts ) >= safe_wind_speed.delay ) {
					Serial.printf( "SAFE WS: %f <= %f\n", sensor_manager->get_sensor_data()->wind_speed, safe_wind_speed.max );
					tmp_is_safe &= true;
				} else
					Serial.printf( "SAFE WS: %f <= %f but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->wind_speed, safe_wind_speed.max, now - safe_wind_speed.ts,  safe_wind_speed.delay);

			}
		} else {
			Serial.printf( "NOT MET : SAFE WS: %f <= %f\n", sensor_manager->get_sensor_data()->wind_speed, safe_wind_speed.max );
			tmp_is_safe &= false;
		}
	}

	if ( safe_cloud_coverage_1.active ) {

		if ( sensor_manager->get_sensor_data()->cloud_coverage <= safe_cloud_coverage_1.max ) {

			if ( !safe_cloud_coverage_1.ts )
				 safe_cloud_coverage_1.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - safe_cloud_coverage_1.ts ) >= safe_cloud_coverage_1.delay ) {
					Serial.printf( "SAFE CC1: %d <= %d\n", sensor_manager->get_sensor_data()->cloud_coverage, safe_cloud_coverage_1.max );
					tmp_is_safe &= true;
				} else {
					Serial.printf( "SAFE CC1: %d <= %d but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->cloud_coverage, safe_cloud_coverage_1.max, now - safe_cloud_coverage_1.ts, safe_cloud_coverage_1.delay  );
				
				}
			}
		} else {
			Serial.printf( "NOT MET : SAFE CC1: %d <= %d\n", sensor_manager->get_sensor_data()->cloud_coverage, safe_cloud_coverage_1.max );
			tmp_is_safe &= false;
		}
	}

	if ( safe_cloud_coverage_2.active ) {

		if ( sensor_manager->get_sensor_data()->cloud_coverage <= safe_cloud_coverage_2.max ) {

			if ( !safe_cloud_coverage_2.ts )
				 safe_cloud_coverage_2.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - safe_cloud_coverage_2.ts ) >= safe_cloud_coverage_2.delay ) {
					Serial.printf( "SAFE CC2: %d <= %d\n", sensor_manager->get_sensor_data()->cloud_coverage, safe_cloud_coverage_2.max );
					tmp_is_safe &= true;
				} else {
					Serial.printf( "SAFE CC2: %d <= %d but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->cloud_coverage, safe_cloud_coverage_2.max, now - safe_cloud_coverage_2.ts, safe_cloud_coverage_2.delay  );
				
				}
			}
		} else {
			Serial.printf( "NOT MET : SAFE CC2: %d <= %d\n", sensor_manager->get_sensor_data()->cloud_coverage, safe_cloud_coverage_2.max );
			tmp_is_safe &= false;
		}
	}

	if ( safe_rain_intensity.active ) {

		if ( sensor_manager->get_sensor_data()->rain_intensity <= safe_rain_intensity.max ) {

			if ( !safe_rain_intensity.ts )
				 safe_rain_intensity.ts = sensor_manager->get_sensor_data()->timestamp;
			else {
				if (( now - safe_rain_intensity.ts ) >= safe_rain_intensity.delay ) {
					Serial.printf( "SAFE RI: %d <= %d\n", sensor_manager->get_sensor_data()->rain_intensity, safe_rain_intensity.max );
					tmp_is_safe &= true;
				} else
					Serial.printf( "SAFE RI: %d <= %d but not yet (%d < %d)\n", sensor_manager->get_sensor_data()->rain_intensity, safe_rain_intensity.max, now - safe_rain_intensity.ts,  safe_rain_intensity.delay);

			}
		} else {
			Serial.printf( "NOT MET: SAFE RI: %d <= %d\n", sensor_manager->get_sensor_data()->rain_intensity, safe_rain_intensity.max );
			tmp_is_safe &= false;
		}
	}

	Serial.printf(" ==> SAFE=%d UNSAFE=%d\n", tmp_is_safe, tmp_is_unsafe );
	
	if ( !tmp_is_safe || tmp_is_unsafe )
		dome->close_shutter();
	else if ( !tmp_is_unsafe && tmp_is_safe )
		dome->open_shutter();
}

void AWSLookout::loop( void * )	// NOSONAR
{
	while( true ) {
		
		if ( enabled )
			check_rules();
		delay( 1000 );	
	}
}

void AWSLookout::initialise( AWSConfig *_config, AWSSensorManager *_mngr, Dome *_dome, bool _debug_mode )
{
	debug_mode = _debug_mode;
	dome = _dome;
	sensor_manager = _mngr;

	Serial.printf( "[INFO] Initialising lookout.\n" );

	initialise_rules( _config );

	std::function<void(void *)> _loop = std::bind( &AWSLookout::loop, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
			std::function<void(void*)>* periodic_tasks_proxy = static_cast<std::function<void(void*)>*>( param );	// NOSONAR
			(*periodic_tasks_proxy)( NULL );
		}, "AWSLookout Task", 4000, &_loop, 5, &watcher_task_handle, 1 );

	enabled = true;
	Serial.printf( "[INFO] Lookout enabled.\n" );
}

void AWSLookout::initialise_rules( AWSConfig *_config )
{
	unsafe_wind_speed_1.active = _config->get_parameter<bool>( "unsafe_wind_speed_active_1" );
	unsafe_wind_speed_1.max = _config->get_parameter<float>( "unsafe_wind_speed_max_1" );
	unsafe_wind_speed_1.delay = _config->get_parameter<int>( "unsafe_wind_speed_delay_1" );
	unsafe_wind_speed_1.ts = 0;
	
	unsafe_wind_speed_2.active = _config->get_parameter<bool>( "unsafe_wind_speed_active_2" );
	unsafe_wind_speed_2.max =  _config->get_parameter<float>( "unsafe_wind_speed_max_2" );
	unsafe_wind_speed_2.delay =  _config->get_parameter<int>( "unsafe_wind_speed_delay_2" );
	unsafe_wind_speed_2.ts = 0;

	unsafe_cloud_coverage_1.active = _config->get_parameter<bool>( "unsafe_cloud_coverage_active_1" );
	unsafe_cloud_coverage_1.max = _config->get_parameter<int>( "unsafe_cloud_coverage_max_1" );
	unsafe_cloud_coverage_1.delay = _config->get_parameter<int>( "unsafe_cloud_coverage_delay_1" );
	unsafe_cloud_coverage_1.ts = 0;
	
	unsafe_cloud_coverage_2.active = _config->get_parameter<bool>( "unsafe_cloud_coverage_active_2" );
	unsafe_cloud_coverage_2.max = _config->get_parameter<int>( "unsafe_cloud_coverage_max_2" );
	unsafe_cloud_coverage_2.delay = _config->get_parameter<int>( "unsafe_cloud_coverage_delay_2" );
	unsafe_cloud_coverage_2.ts = 0;

	unsafe_rain_intensity.active = _config->get_parameter<bool>( "unsafe_rain_intensity_active" );
	unsafe_rain_intensity.max = _config->get_parameter<int>( "unsafe_rain_intensity_max" );

	safe_wind_speed.active = _config->get_parameter<bool>( "safe_wind_speed_active" );
	safe_wind_speed.max = _config->get_parameter<float>( "safe_wind_speed_max" );
	safe_wind_speed.delay = _config->get_parameter<int>( "safe_wind_speed_delay" );
	safe_wind_speed.ts = 0;

	safe_cloud_coverage_1.active = _config->get_parameter<bool>( "safe_cloud_coverage_active_1" );
	safe_cloud_coverage_1.max = _config->get_parameter<int>( "safe_cloud_coverage_max_1" );
	safe_cloud_coverage_1.delay = _config->get_parameter<int>( "safe_cloud_coverage_delay_1" );
	safe_cloud_coverage_1.ts = 0;
	
	safe_cloud_coverage_2.active = _config->get_parameter<bool>( "safe_cloud_coverage_active_2" );
	safe_cloud_coverage_2.max = _config->get_parameter<int>( "safe_cloud_coverage_max_2" );
	safe_cloud_coverage_2.delay = _config->get_parameter<int>( "safe_cloud_coverage_delay_2" );
	safe_cloud_coverage_2.ts = 0;

	safe_rain_intensity.active = _config->get_parameter<bool>( "safe_rain_intensity_active" );
	safe_rain_intensity.max = _config->get_parameter<int>( "safe_rain_intensity_max" );
	safe_rain_intensity.delay = _config->get_parameter<int>( "safe_rain_intensity_delay" );
	safe_rain_intensity.ts = 0;
}

void AWSLookout::set_rain_event()
{
	rain_event = true;
}
