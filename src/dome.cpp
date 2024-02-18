/*
  	dome.cpp

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

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <AsyncUDP_ESP32_W5500.hpp>
#include <ESPAsyncWebSrv.h>

#include "gpio_config.h"
#include "common.h"
#include "SC16IS750.h"
#include "device.h"
#include "dome.h"
#include "AstroWeatherStation.h"

extern void IRAM_ATTR		_handle_dome_shutter_is_moving( void );

extern AstroWeatherStation	station;
extern time_t 				last_ntp_time;
extern uint16_t				ntp_time_misses;

//
// IMPORTANT: Some indications about dome shutter status handling by this driver
//
//	==> There are 4 pins to control and monitor the dome shutter
//
//		Control: two independant systems can be attached, for redundancy, and they are connected to a pull-down resistor (the relay will switch when the signal is down)
//				GPIO_DOME_1 & GPIO_DOME_2 if a SC16IS750 GPIO extender is used
//				GPIO_DOME_1_DIRECT & GPIO_DOME_2_DIRECT if not
//
//				The DOME SHUTTER will be kept OPEN if and only if BOTH PINS levels are HIGH
//					==> if the station or one of the PINS is DOWN, the dome will be CLOSED no matter what.
//
//		Status:
//				GPIO_DOME_STATUS, connected to a pull-down resistor
//						if pin level is DOWN, the dome shutter is expected to be OPEN
//							==> it requires an active action (pulling HIGH the pin level) to indicate that the shutter is CLOSED
//				GPIO_DOME_MOVING, connected to a pull-down resistor 
//						as long as the level is down, the dome shutter does not move
//						if the pin level is RISING, then the dome shutter has been ordered to MOVE, a FALLING edge is not handled.
//							if the initial status is closed, we consider it is opening and vice-versa
//

Dome::Dome( void )
{
	set_name( "Roll-off roof" );
	set_description( "Generic relay driven roll-off roof" );
	set_driver_version( "1.0" );
}

void Dome::check_guard_times( void )
{
	time_t now = station.get_timestamp();

	if ( !catch_shutter_moving && (( now - shutter_moving_ts ) > dome_shutter_moving_guard_time )) {
		
		attachInterrupt( GPIO_DOME_MOVING, _handle_dome_shutter_is_moving, RISING );
		catch_shutter_moving = true;

	}
}

bool Dome::get_shutter_closed_status( void )
{
	bool x = ( digitalRead( GPIO_DOME_STATUS ) == HIGH );

	if ( x )
		shutter_status = dome_shutter_status_t::Closed;

	if ( get_debug_mode() )
		Serial.printf( "[DEBUG] Dome shutter closed : %s\n", x ? "yes" : "no" );

	return x;
}

void Dome::close_shutter( void )
{
	Serial.printf( "[INFO] Closing dome shutter.\n" );
	if ( sc16is750 ) {

		while ( xSemaphoreTake( i2c_mutex, 50 / portTICK_PERIOD_MS ) != pdTRUE );

		sc16is750->digitalWrite( GPIO_DOME_1, LOW );
		sc16is750->digitalWrite( GPIO_DOME_2, LOW );

		xSemaphoreGive( i2c_mutex );

	} else {
	
		digitalWrite( GPIO_DOME_1_DIRECT, LOW );
		digitalWrite( GPIO_DOME_2_DIRECT, LOW );

	}
	do_close_shutter = false;
	// Do not update shutter_status, wait for a rising edge on the monitoring pin
}

void Dome::control_task( void * )	// NOSONAR
{
	while ( true ) {

			if ( do_close_shutter )
				close_shutter();
			else {
				if ( do_open_shutter )
					open_shutter();
			}
			check_guard_times();
			if ( _shutter_is_moving && catch_shutter_moving ) {

					time( &shutter_moving_ts );
					switch( shutter_status ) {
		
						case dome_shutter_status_t::Closed:
						case dome_shutter_status_t::Opening:
							shutter_status = dome_shutter_status_t::Opening;
							break;
						case dome_shutter_status_t::Open:
						case dome_shutter_status_t::Closing:
							shutter_status = dome_shutter_status_t::Closing;
							break;
						break;
				}
				_shutter_is_moving = false;
				catch_shutter_moving = false;
			}
			delay( 500 );
	}
}

void Dome::initialise( uint16_t _dome_shutter_moving_guard_time, bool _debug_mode )
{
	set_debug_mode( _debug_mode );
	dome_shutter_moving_guard_time = _dome_shutter_moving_guard_time;
Serial.printf("INIT DOME\n");
	if ( sc16is750 ) {
		
		pinMode( GPIO_DOME_1, OUTPUT );
		pinMode( GPIO_DOME_2, OUTPUT );

	} else {

Serial.printf("PIN MODE\n");
		pinMode( GPIO_DOME_1_DIRECT, OUTPUT );
		pinMode( GPIO_DOME_2_DIRECT, OUTPUT );
		delay(5000);
	}

	pinMode( GPIO_DOME_MOVING, INPUT );
	pinMode( GPIO_DOME_STATUS, INPUT );
	
	get_shutter_closed_status();
	
	attachInterrupt( GPIO_DOME_MOVING, _handle_dome_shutter_is_moving, RISING );
	catch_shutter_moving = true;

	std::function<void(void *)> _control = std::bind( &Dome::control_task, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
            std::function<void(void*)>* control_proxy = static_cast<std::function<void(void*)>*>( param );
            (*control_proxy)( NULL );
		}, "DomeControl", 2000, &_control, configMAX_PRIORITIES - 2, &dome_task_handle, 1 );
}

void Dome::initialise( I2C_SC16IS750 *_sc16is750, SemaphoreHandle_t _i2c_mutex, uint16_t _dome_shutter_moving_guard_time, bool _debug_mode )
{
	i2c_mutex =_i2c_mutex;
	sc16is750 = _sc16is750;
	initialise( _dome_shutter_moving_guard_time, _debug_mode );
}

void Dome::open_shutter( void )
{
	do_close_shutter = false;
	do_open_shutter = false;
	
	Serial.printf( "[INFO] Opening dome shutter.\n" );
	if ( sc16is750 ) {

		while ( xSemaphoreTake( i2c_mutex, 50 / portTICK_PERIOD_MS ) != pdTRUE );

		sc16is750->digitalWrite( GPIO_DOME_1, HIGH );
		sc16is750->digitalWrite( GPIO_DOME_2, HIGH );

		xSemaphoreGive( i2c_mutex );

	} else {

		digitalWrite( GPIO_DOME_1_DIRECT, HIGH );
		digitalWrite( GPIO_DOME_2_DIRECT, HIGH );

	}
}

void Dome::shutter_is_moving( void )
{
	_shutter_is_moving = true;
	time( &shutter_moving_ts );
	detachInterrupt( GPIO_DOME_MOVING );
}

void Dome::trigger_close_shutter( void )
{
	do_close_shutter = true;
}
