/*
   NXP SC16IS1750 UART Bridge driver
   
   (c) 2023 Lesage F. 

	Revisions
		1.0.0	: Barebone version, polled mode operation, made to work with the AstroWeatherStation's GPS

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

/*
 * Used the following documents
 * 		Product data sheet: https://www.nxp.com/docs/en/data-sheet/SC16IS740_750_760.pdf
 * 		
 * I2C mode only for the moment, SPI will come later :-)
 * 
 */

#undef CONFIG_DISABLE_HAL_LOCKS
#define _ASYNC_WEBSERVER_LOGLEVEL_       0
#define _ETHERNET_WEBSERVER_LOGLEVEL_      0

#include <Arduino.h>
#include <Wire.h>
#include "SC16IS750.h"

I2C_SC16IS750::I2C_SC16IS750( uint8_t _address )
{
	address = _address >> 1;		// 8 bits addressing (address > 0x77, see Table 32 of data sheet), we have to drop the R/W bit
	has_peek = 0;
	peek_byte = -1;
}

int I2C_SC16IS750::available( void )
{
    return available_data_in_FIFO();
}

bool I2C_SC16IS750::begin( uint32_t baudrate )
{
	Wire.begin();
	if ( !test() ) {

		Wire.end();
		return false;
	}

	toggle_sleep( false );
    FIFO_reset( true, true );
    delay(10);
    FIFO_enable( true );
	enable_extra_features();
	enable_tcr_tlr();
	set_flow_control_levels( 48, 24 );
	enable_TX_RX();
    set_line( 8, 0, 1 );
	set_baudrate( baudrate );
    return true;
}

void I2C_SC16IS750::enable_TX_RX( void )
{
	uint8_t tmp = read_register( SC16IS750_EFCR );

	write_register( SC16IS750_EFCR, tmp & (uint8_t)(~( EFCR_RX_DISABLE | EFCR_TX_DISABLE )) );
}

void I2C_SC16IS750::set_flow_control_levels( uint8_t halt_level, uint8_t resume_level )
{
	uint8_t val = (( halt_level / 4 ) & 0x0F ) + (( (resume_level / 4) & 0x0F ) << 4 );

	write_register( SC16IS750_TCR, val );
}

void I2C_SC16IS750::enable_tcr_tlr( void )
{
	uint8_t	tmp = read_register( SC16IS750_MCR );

	write_register( SC16IS750_MCR, tmp | MCR_TCR_TLR_ENABLE );
}

void I2C_SC16IS750::toggle_sleep( bool sleep )
{
	uint8_t	tmp = read_register( SC16IS750_IER );

	if ( sleep )
		write_register( SC16IS750_IER, tmp | IER_SLEEP_MODE_ENABLE );
	else
		write_register( SC16IS750_IER, tmp & (uint8_t)(~IER_SLEEP_MODE_ENABLE) );
}

void I2C_SC16IS750::enable_extra_features( void )
{
	uint8_t	tmp,
			tmp2;

	tmp = read_register( SC16IS750_LCR );
	write_register( SC16IS750_LCR, LCR_ACCESS_EFR );
	
	tmp2 = read_register( SC16IS750_EFR );
	write_register( SC16IS750_EFR, tmp2 | EFR_ENHANCED_FUNC_ENABLE );

	write_register( SC16IS750_LCR, tmp );
}

bool I2C_SC16IS750::test( void )
{
	write_register( SC16IS750_SPR, 0x41 );
	return ( read_register( SC16IS750_SPR ) == 0x41 );
}

int8_t I2C_SC16IS750::read( void )
{
	return read_byte();
}

uint8_t I2C_SC16IS750::write( uint8_t b )
{
    return write_byte( b );
}

uint8_t I2C_SC16IS750::write_byte( uint8_t b )
{
	while (( read_register( SC16IS750_LSR ) & LSR_OVERRUN_ERROR )) { };

	return write_register( SC16IS750_THR, b );
}

int8_t I2C_SC16IS750::read_byte( void )
{
	return read_register( SC16IS750_RHR );
}

void I2C_SC16IS750::set_baudrate( uint32_t baudrate )
{
    ulong		divisor;
    uint8_t		prescaler = 1,
				tmp1,
				tmp2;

	divisor = SC16IS750_XTAL_FREQ / ( 16 * baudrate );
	if ( divisor > 0xffff ) {
		
		prescaler = 4;		// since DLL/DLH are both 8 bits, anything > 0xffff needs to be further divided
		divisor /= 4;
	}

	tmp1 = read_register( SC16IS750_LCR );
	write_register( SC16IS750_LCR, tmp1 | LCR_DIVISOR_LATCH_ENABLE ); 	// DLL & DLH are available only if LCR[7] is set

    write_register( SC16IS750_DLL, (uint8_t)(divisor & 0xFF) );
    write_register( SC16IS750_DLH, (uint8_t)(divisor >> 8) );

    write_register( SC16IS750_LCR, tmp1 );

	tmp1 = read_register( SC16IS750_EFR );
	write_register( SC16IS750_EFR, EFR_ENHANCED_FUNC_ENABLE );		// MCR[7] can be changed if EFR[4] is set
	
	tmp2 = read_register( SC16IS750_MCR );
	if ( prescaler  == 4 )
		write_register( SC16IS750_MCR, tmp2 | MCR_CLOCK_DIVISOR );
	else
		write_register( SC16IS750_MCR, tmp2 & ((uint8_t)(~MCR_CLOCK_DIVISOR )) );

	write_register( SC16IS750_EFR, tmp1 );
}

void I2C_SC16IS750::flush()
{
    FIFO_reset( true, true );
    FIFO_enable( false );
}

int I2C_SC16IS750:: peek()
{
	if ( has_peek )
		return peek_byte;

	// FIXME: set a mutex?
	peek_byte = read_byte();
	has_peek = ( peek_byte >= 0 );
	return peek_byte;
}

void I2C_SC16IS750::set_line( uint8_t word_length, uint8_t parity_type, uint8_t stop_bits )
{
    uint8_t tmp = read_register( SC16IS750_LCR );
    
    tmp &= (uint8_t)(~( LCR_WORD_LENGTH0 | LCR_WORD_LENGTH1 | LCR_STOP_BITS_15_OR_2 | LCR_PARITY_ENABLE | LCR_PARITY_EVEN | LCR_PARITY_FORCE ));
    
	switch ( word_length ) {           
		case 5:
			tmp |= LCR_WORD_LENGTH_5;
			break;
		case 6:
			tmp |= LCR_WORD_LENGTH_6;
			break;
		case 7:
			tmp |= LCR_WORD_LENGTH_7;
			break;
		case 8:
			tmp |= LCR_WORD_LENGTH_8;
			break;
		default:
			tmp |= LCR_WORD_LENGTH_8;
			break;
	}

	if ( stop_bits == 2 )
		tmp |= LCR_STOP_BITS_15_OR_2;

	switch ( parity_type ) {
		case NO_PARITY:
			break;
		case ODD_PARITY:
			tmp |= SET_LCR_PARITY_ODD;
			break;
		case EVEN_PARITY:
			tmp |= SET_LCR_PARITY_EVEN;
			break;
		case FORCE_1_PARITY:
			tmp |= SET_LCR_PARITY_1;
            break;
		case FORCE_0_PARITY:
			tmp |= SET_LCR_PARITY_0;
            break;
		default:
			break;
	}

    write_register( SC16IS750_LCR, tmp );
}

int8_t I2C_SC16IS750::read_register( uint8_t register_address )
{
    uint8_t n;

	Wire.beginTransmission( address );
	Wire.write( ( register_address << SC16IS750_REG_ADDR_SHIFT ));	// Product sheet, table 33. Register address byte: use bits 3-6 and bits 2&1 must be 0, bit 0 is not used.
	switch( Wire.endTransmission( 0 )) {
		case 0:
			break;
		case 1:
			Serial.printf("ERROR: SC16IS750 transmission failed: data to long to fit in buffer\n" );
			break;
		case 2:
			Serial.printf("ERROR: SC16IS750 transmission failed: NACK on transmit of address\n" );
			break;
		case 3:
			Serial.printf("ERROR: SC16IS750 transmission failed: NACK on transmit of data\n" );
			break;
		case 4:
			Serial.printf("ERROR: SC16IS750 transmission failed: other error\n" );
			break;
		case 5:
			Serial.printf("ERROR: SC16IS750 transmission failed: timeout\n" );
			break;
	}
	if ( ( n = Wire.requestFrom( address, 1 )) != 1 )
		Serial.printf( "ERROR: SC16IS750 received %d bytes instead of 1 while addressing register 0x%02x\n", n, register_address );
	return Wire.read();
}

bool I2C_SC16IS750::write_register (uint8_t register_address, uint8_t value )
{
	Wire.beginTransmission( address );
	Wire.write( ( register_address << SC16IS750_REG_ADDR_SHIFT ));	// Product sheet, table 33. Register address byte: use bits 3-6 and bits 2&1 must be 0, bit 0 is not used.
	Wire.write( value );
	switch ( Wire.endTransmission( 1 )) {
		case 0:
			return true;
			break;
		case 1:
			Serial.printf("ERROR: SC16IS750 transmission failed: data to long to fit in buffer\n" );
			break;
		case 2:
			Serial.printf("ERROR: SC16IS750 transmission failed: NACK on transmit of address\n" );
			break;
		case 3:
			Serial.printf("ERROR: SC16IS750 transmission failed: NACK on transmit of data\n" );
			break;
		case 4:
			Serial.printf("ERROR: SC16IS750 transmission failed: other error\n" );
			break;
		case 5:
			Serial.printf("ERROR: SC16IS750 transmission failed: timeout\n" );
			break;
	}
    return false;
}

void I2C_SC16IS750::FIFO_enable( bool enable_fifo )
{
	uint8_t tmp = read_register( SC16IS750_FCR );

	if ( enable_fifo )
		write_register( SC16IS750_FCR, tmp | FCR_FIFO_ENABLE );
	else
		write_register( SC16IS750_FCR, tmp & ((uint8_t)(~FCR_FIFO_ENABLE)) );  
}

bool I2C_SC16IS750::available_data_in_FIFO( void )
{
	return (( read_register( SC16IS750_LSR ) & 0x01 ) == 0x01 );	// Polled mode operation
}


void I2C_SC16IS750::FIFO_reset( bool rx, bool tx )
{
     uint8_t tmp = read_register( SC16IS750_FCR );

    if ( rx )
        tmp |= FCR_FIFO_RX_RESET;
    if ( tx )
        tmp |= FCR_FIFO_TX_RESET;
  
    write_register( SC16IS750_FCR, tmp );
}

//
// GPIO handling
//

bool I2C_SC16IS750::set_IO_direction( uint8_t pin, bool write )
{
	uint8_t tmp = read_register( SC16IS750_IODIR );

	if ( write )
		return write_register( SC16IS750_IODIR, tmp  | ( 1 << pin )  );
	else
		return write_register( SC16IS750_IODIR, tmp  & ~( 1 << pin )  );
}

bool I2C_SC16IS750::pinMode( uint8_t pin, bool write )
{
	return set_IO_direction( pin, write ); 
}

uint8_t I2C_SC16IS750::get_IO_state( void )
{
	return read_register( SC16IS750_IOSTATE );
}

bool I2C_SC16IS750::digitalRead( uint8_t pin )
{
	uint8_t pinStates = get_IO_state();

	if ( pin > 7 )
		return false;

	return (( pinStates & (1 << pin) ) != 0 );
}

bool I2C_SC16IS750::setup_GPIO_interrupt( uint8_t pin, bool enable )
{
	uint8_t tmp = read_register( SC16IS750_IOINTENA );

	if ( enable )
		return write_register( SC16IS750_IOINTENA, tmp | (1 << pin));
	else
		return write_register( SC16IS750_IOINTENA, tmp & ~(1 << pin));

}

bool I2C_SC16IS750::digitalWrite( uint8_t pin, bool high )
{
	uint8_t tmp = get_IO_state();

	if ( high )
		return write_register( SC16IS750_IOSTATE, tmp  | (1 << pin)  );
	else
		return write_register( SC16IS750_IOSTATE, tmp  & ~(1 << pin)  );
}
