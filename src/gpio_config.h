/*	
  	gpio_config.h
  	
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
#ifndef _GPIO_CONFIG_H
#define _GPIO_CONFIG_H

#ifndef _defaults_H
#include "defaults.h"
#endif

// Misc
#define GPIO_DEBUG		GPIO_NUM_34

// Wind sensors
#define GPIO_WIND_SENSOR_RX		GPIO_NUM_32	// RO Pin
#define GPIO_WIND_SENSOR_TX		GPIO_NUM_25	// DI Pin
#define GPIO_WIND_SENSOR_CTRL	GPIO_NUM_33	// RE+DE Pins

// Rain sensor
#define GPIO_RAIN_SENSOR_RX		GPIO_NUM_17	// J2 SO PIN
#define GPIO_RAIN_SENSOR_TX		GPIO_NUM_16	// J2 SI PIN
#define GPIO_RAIN_SENSOR_RAIN	GPIO_NUM_35	// J1 OUT PIN

#if DEFAULT_HAS_ETHERNET == 0
#define GPIO_RAIN_SENSOR_MCLR	GPIO_NUM_23	// J2 MCLR PIN
#else
#define GPIO_RAIN_SENSOR_MCLR	GPIO_NUM_15	// J2 MCLR PIN
#endif

// MOSFET SWITCHES
#define GPIO_ENABLE_3_3V		GPIO_NUM_5
#define GPIO_ENABLE_12V			GPIO_NUM_18

// Battery level
#define GPIO_BAT_ADC			GPIO_NUM_13
#define GPIO_BAT_ADC_EN			GPIO_NUM_12

// Ethernet W5500
#define	GPIO_SPI_MOSI			GPIO_NUM_23
#define	GPIO_SPI_MISO			GPIO_NUM_19
#define	GPIO_SPI_SCK			GPIO_NUM_18
#define	GPIO_SPI_CS_ETH			GPIO_NUM_5
#define	GPIO_SPI_INT			GPIO_NUM_4

// SDCard reader
#define	GPIO_SPI_CS_SD			GPIO_NUM_2

// Dome control
const	uint8_t	GPIO_DOME_1			= 0;
const	uint8_t	GPIO_DOME_2			= 1;
const	uint8_t	GPIO_DOME_1_DIRECT	= 13;
const	uint8_t	GPIO_DOME_2_DIRECT	= 14;

#define	GPIO_DOME_STATUS		GPIO_NUM_39
#define	GPIO_DOME_MOVING		GPIO_NUM_36

// Direct connection between GPS and ESP32 (not through SC16IS750)
#define	GPS_RX					GPIO_NUM_27
#define	GPS_TX					GPIO_NUM_26

#endif
