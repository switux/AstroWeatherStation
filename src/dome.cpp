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
extern void IRAM_ATTR		_handle_dome_shutter_closed_change( void );

extern AstroWeatherStation	station;
extern time_t 				last_ntp_time;		// NOSONAR
extern uint16_t				ntp_time_misses;	// NOSONAR

//
// IMPORTANT: Some indications about dome shutter status handling by this driver
//
//	==> There are 4 pins to control and monitor the dome shutter
//
//		Control: two independant systems can be attached, for redundancy, and they are connected to a pull-down resistor (the relay will trigger when the signal is down)
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

bool Dome::get_shutter_closed_status( void )
{
	if ( !is_connected ) {

		Serial.printf( "[DOME      ] [ERROR] Dome is not initialised, cannot get shutter closed status!\n" );
		return false;
	}

	bool x = ( digitalRead( GPIO_DOME_STATUS ) == HIGH );

	if ( x ) {

		dome_data->shutter_status = dome_shutter_status_t::Closed;
		dome_data->closed_sensor = true;

	} else {

		dome_data->shutter_status = dome_shutter_status_t::Open;
		dome_data->closed_sensor = false;

	}

	if ( get_debug_mode() )
		Serial.printf( "[DOME      ] [DEBUG] Dome shutter closed : %s\n", x ? "yes" : "no" );

	return x;
}

bool Dome::get_shutter_moving_status( void )
{
	return dome_data->moving_sensor = ( digitalRead( GPIO_DOME_MOVING ) == HIGH );
}

bool Dome::close_shutter( void )
{
	if ( !is_connected ) {

		Serial.printf( "[DOME      ] [ERROR] Dome is not initialised, cannot close shutter!\n" );
		return false;
	}

	dome_data->close_command = true;

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
	return true;
}

void Dome::control_task( void *dummy )	// NOSONAR
{
	while ( true ) {

			if ( do_close_shutter )
				close_shutter();
			else {
				if ( do_open_shutter )
					open_shutter();
			}

			if ( shutter_is_moving  ) {

					switch( shutter_status ) {

						case dome_shutter_status_t::Closed:
						case dome_shutter_status_t::Opening:
							dome_data->shutter_status = shutter_status = dome_shutter_status_t::Opening;
							break;
						case dome_shutter_status_t::Open:
						case dome_shutter_status_t::Closing:
							dome_data->shutter_status = shutter_status = dome_shutter_status_t::Closing;
							break;

				}
				shutter_is_moving = false;
			}
			delay( 1000 );
			dome_data->moving_sensor = false;
			get_shutter_closed_status();
			get_shutter_moving_status();
	}
}

void Dome::initialise( dome_data_t *_dome_data, bool _debug_mode )
{
	set_debug_mode( _debug_mode );
	dome_data = _dome_data;

	if ( sc16is750 ) {

		pinMode( GPIO_DOME_1, OUTPUT );
		digitalWrite( GPIO_DOME_1, HIGH );
		pinMode( GPIO_DOME_2, OUTPUT );
		digitalWrite( GPIO_DOME_2, HIGH );

	} else {

		pinMode( GPIO_DOME_1_DIRECT, OUTPUT );
		digitalWrite( GPIO_DOME_1_DIRECT, HIGH );
		pinMode( GPIO_DOME_2_DIRECT, OUTPUT );
		digitalWrite( GPIO_DOME_2_DIRECT, HIGH );
	}

	pinMode( GPIO_DOME_MOVING, INPUT );
	pinMode( GPIO_DOME_STATUS, INPUT );

	if ( !get_shutter_closed_status() )
		dome_data->shutter_status = shutter_status = dome_shutter_status_t::Open;

	dome_data->moving_sensor = digitalRead( GPIO_DOME_MOVING );
	dome_data->closed_sensor = digitalRead( GPIO_DOME_STATUS );
	dome_data->close_command = false;

	attachInterrupt( GPIO_DOME_STATUS, _handle_dome_shutter_closed_change, CHANGE );
	attachInterrupt( GPIO_DOME_MOVING, _handle_dome_shutter_is_moving, RISING );

	std::function<void(void *)> _control = std::bind( &Dome::control_task, this, std::placeholders::_1 );
	xTaskCreatePinnedToCore(
		[](void *param) {	// NOSONAR
            std::function<void(void*)>* control_proxy = static_cast<std::function<void(void*)>*>( param );
            (*control_proxy)( NULL );
		}, "DomeControl", 2000, &_control, configMAX_PRIORITIES - 2, &dome_task_handle, 1 );

	is_connected = true;
}

void Dome::initialise( I2C_SC16IS750 *_sc16is750, SemaphoreHandle_t _i2c_mutex, dome_data_t *_dome_data, bool _debug_mode )
{
	i2c_mutex =_i2c_mutex;
	sc16is750 = _sc16is750;
	initialise( _dome_data, _debug_mode );
}

bool Dome::open_shutter( void )
{
	if ( !is_connected ) {

		Serial.printf( "[DOME      ] [ERROR] Dome is not initialised, cannot open shutter!\n" );
		return false;
	}

	dome_data->close_command = close_shutter_command = do_close_shutter = false;
	do_open_shutter = false;

	if ( sc16is750 ) {

		while ( xSemaphoreTake( i2c_mutex, 50 / portTICK_PERIOD_MS ) != pdTRUE );

		sc16is750->digitalWrite( GPIO_DOME_1, HIGH );
		sc16is750->digitalWrite( GPIO_DOME_2, HIGH );

		xSemaphoreGive( i2c_mutex );

	} else {

		digitalWrite( GPIO_DOME_1_DIRECT, HIGH );
		digitalWrite( GPIO_DOME_2_DIRECT, HIGH );

	}
	return true;
}

void Dome::shutter_closed_change( void )
{
	if ( digitalRead( GPIO_DOME_STATUS ))
		dome_data->shutter_status = shutter_status = dome_shutter_status_t::Closed;
	else
		dome_data->shutter_status = shutter_status = dome_shutter_status_t::Opening;
}

void Dome::set_shutter_is_moving( void )
{
	dome_data->moving_sensor = shutter_is_moving = true;
}

void Dome::trigger_close_shutter( void )
{
	close_shutter_command = do_close_shutter = true;
}
