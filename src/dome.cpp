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

#include "gpio_config.h"
#include "SC16IS750.h"
#include "device.h"
#include "dome.h"

extern void IRAM_ATTR		_handle_dome_shutter_is_moving( void );

//
// IMPORTANT: Some indications about dome shutter status handling by this driver
//
//	==> There are 4 pins to control and monitor the dome shutter
//
//		Control: two independant systems can be attached, for redundancy, and they are connected to a pull-down resistor
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

void Dome::close( void )
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
	close_dome = false;
	// Do not update shutter_status, wait for a rising edge on the monitoring pin
}

bool Dome::closed( void )
{
	bool x = ( digitalRead( GPIO_DOME_STATUS ) == HIGH );

	if ( x )
		shutter_status = dome_shutter_status_t::Closed;

	if ( get_debug_mode() )
		Serial.printf( "[DEBUG] Dome shutter closed : %s\n", x ? "yes" : "no" );

	return x;
}

void Dome::control_task( void * )	// NOSONAR
{
	while ( true ) {

			if ( close_dome )
				close();
			else {
				if ( open_dome )
					open();
			}
			delay( 500 );
	}
}

void Dome::initialise( bool _debug_mode )
{
	set_debug_mode( _debug_mode );
	
	// Controlling the dome from an ISR just does not work because the wdt fires off
	// during I2C communication with the SC16IS750
	// Instead we just toggle a flag which is then read by a high prio task in charge of
	// closing the dome.

	attachInterrupt( GPIO_DOME_MOVING, _handle_dome_shutter_is_moving, RISING );

	std::function<void(void *)> _control = std::bind( &Dome::control_task, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
            std::function<void(void*)>* control_proxy = static_cast<std::function<void(void*)>*>( param );
            (*control_proxy)( NULL );
		}, "DomeControl", 2000, &_control, configMAX_PRIORITIES - 2, &dome_task_handle, 1 );
}

void Dome::initialise( I2C_SC16IS750 *_sc16is750, SemaphoreHandle_t _i2c_mutex, bool _debug_mode )
{
	i2c_mutex =_i2c_mutex;
	sc16is750 = _sc16is750;
	initialise( _debug_mode );
}

void Dome::open( void )
{
	close_dome = false;
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
	time( &shutter_status_change_ts );
	
	switch( shutter_status ) {
		
		case dome_shutter_status_t::Closed:
		case dome_shutter_status_t::Opening:
			shutter_status = dome_shutter_status_t::Opening;
			break;
		case dome_shutter_status_t::Open:
		case dome_shutter_status_t::Closing:
			shutter_status = dome_shutter_status_t::Closing;
			break;
		case dome_shutter_status_t::Error:
			closed();
			break;
	}
}

void Dome::trigger_close( void )
{
  close_dome = true;
}
