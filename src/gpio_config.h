#pragma once
#ifndef _GPIO_CONFIG_H
#define _GPIO_CONFIG_H

// Wind sensors
#define GPIO_WIND_VANE_RX		GPIO_NUM_26	// RO Pin
#define GPIO_WIND_VANE_TX		GPIO_NUM_14	// DI Pin
#define GPIO_WIND_VANE_CTRL		GPIO_NUM_27	// RE+DE Pins

#define GPIO_ANEMOMETER_RX		GPIO_NUM_33	// RO Pin
#define GPIO_ANEMOMETER_TX		GPIO_NUM_25	// DI Pin
#define GPIO_ANEMOMETER_CTRL	GPIO_NUM_32	// RE+DE Pins

// FIXME: this is not compatible with W5500 / POE
// Rain sensor
#define GPIO_RG9_RX				GPIO_NUM_17	// J2 SO PIN
#define GPIO_RG9_TX				GPIO_NUM_16	// J2 SI PIN
#define GPIO_RG9_MCLR			GPIO_NUM_23	// J2 MCLR PIN
#define GPIO_RG9_RAIN			GPIO_NUM_35	// J1 OUT PIN

// MOSTFET SWITCHES
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

#endif
