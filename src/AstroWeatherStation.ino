/*	
  	AstroWeatherStation.ino

	(c) 2023 F.Lesage

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

#include <ArduinoJson.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>
#include <thread>
#include <TinyGPSPlus.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include <SoftwareSerial.h>

extern const char *_anemometer_model[2];
extern const char *_windvane_model[2];

#include "gpio_config.h"
#include "GPS.h"
#include "AstroWeatherStation.h"
#include "SQM.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "SC16IS750.h"
#include "AWSSensorManager.h"
#include "alpaca.h"
#include "AWS.h"

AstroWeatherStation station;

void setup()
{
	Serial.begin( 115200 );
	delay( 500 );	// TODO: not sure we really need it, except maybe after a firmware update via USB, to give some time to the serial monitor

	if ( !station.initialise()) {
		
		Serial.printf( "[PANIC] ===> AstroWeatherStation did not properly initialise. Stopping here! <===\n" );
		while( true ) { delay( 100000 ); }
	}

	if ( station.on_solar_panel() ) {

		if ( !station.is_rain_event() ) {

			station.read_sensors();
			station.send_data();
			station.check_ota_updates();
		}

		Serial.printf( "[INFO] Entering sleep mode.\n" );

		esp_sleep_enable_timer_wakeup( US_SLEEP );
		esp_sleep_enable_ext0_wakeup( GPIO_RG9_RAIN, LOW );
		esp_deep_sleep_start();
	}
}

void loop()
{
	if ( station.on_solar_panel() ) {

		Serial.printf( "[PANIC] Must not execute this code when running on solar panel!\n" );
		while (true) { delay( 10000 ); }
	}


	while( true ) {

		station.read_sensors();
		station.send_data();
		station.check_ota_updates();
		delay( 1000 * 60 * 5 );
		//delay( 1000 * 1 * 5 );
	}
}

void IRAM_ATTR _handle_rain_event( void )
{
	station.handle_rain_event();
}
