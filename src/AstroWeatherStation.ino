/*	
  	AstroWeatherStation.ino

	(c) 2023-2024 F.Lesage

	1.0.x - Initial version.
	1.1.x - Refactored to remove unnecessary global variables
	1.2.x - Added experimental SQM feature
	1.3.x - Added OTA Update / Rudimentary configuration webserver / Persistent configuration
	2.0.x - Added button to enter configuration mode
	3.0.0 - Complete rework to become more modular
	
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
#include "soc/rtc_wdt.h"

#include "gpio_config.h"
#include "common.h"
#include "AstroWeatherStation.h"

extern char			catch_rain_event;

const etl::string<12>		REV 				= "3.0.0";
const unsigned long 		US_SLEEP			= 5 * 60 * 1000000;					// 5 minutes
const unsigned long long	US_HIBERNATE		= 1 * 24 * 60 * 60 * 1000000ULL;	// 1 day

AstroWeatherStation station;

void setup()
{
	Serial.begin( 115200 );

	delay( 500 );	// TO CHECK: not sure we really need it, except maybe after a firmware update via USB, to give some time to the serial monitor

	if ( !station.initialise()) {
		
		Serial.printf( "[PANIC] ===> AstroWeatherStation did not properly initialise. Stopping here! <===\n" );
		while( true ) { delay( 100000 ); }
	}

	if ( station.on_solar_panel() ) {

		if ( !station.is_rain_event() ) {

			station.read_sensors();
			station.send_data();
			if ( station.get_station_data()->health.battery_level > 50 )
				station.check_ota_updates( true );

		} else
		
			station.handle_event( aws_event_t::RAIN );

		esp_sleep_enable_timer_wakeup( US_SLEEP );

		if ( station.has_device( aws_device_t::RAIN_SENSOR ) ) {

			if ( catch_rain_event ) {

				if ( station.get_debug_mode() )
					Serial.printf( "[DEBUG] Monitoring rain sensor.\n" );
				esp_sleep_enable_ext0_wakeup( GPIO_RAIN_SENSOR_RAIN, LOW );

			 } else {

				if ( station.get_debug_mode() )
					Serial.printf( "[DEBUG] Not monitoring rain sensor.\n" );
			 }
		}
		Serial.printf( "[INFO] Entering sleep mode.\n" );
		esp_deep_sleep_start();
	}

}

void loop()
{
	if ( station.on_solar_panel() ) {

		Serial.printf( "[PANIC] Must not execute this code when running on solar panel!\n" );
		while (true) { delay( 10000 ); }
	}

	while( true )
		delay( 1000 * 60 * 1 );
}

void IRAM_ATTR _handle_dome_shutter_is_moving( void )
{
	station.handle_event( aws_event_t::DOME_SHUTTER_MOVING );
}

void IRAM_ATTR _handle_dome_shutter_closed_change( void )
{
	station.handle_event( aws_event_t::DOME_SHUTTER_CLOSED_CHANGE );
}

void IRAM_ATTR _handle_rain_event( void )
{
	station.handle_event( aws_event_t::RAIN );
}
