/*
   NXP SC16IS1750 UART Bridge driver

   (c) 2023-2024 Lesage F.

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
#ifndef	_SC16IS750_H_
#define	_SC16IS750_H_

#include <Arduino.h>

const uint8_t			DEFAULT_SC16IS750_ADDR				= 0x90;

#define	SC16IS750_RHR		(0x00)		// RX FIFO
#define	SC16IS750_THR		(0X00)		// TX FIFO
#define	SC16IS750_IER		(0X01)		// Interrupt enable
#define	SC16IS750_FCR		(0X02)		// FIFO control
#define	SC16IS750_IIR		(0X02)		// Interrupt identification
#define	SC16IS750_LCR		(0X03)		// Line control
#define	SC16IS750_MCR		(0X04)		// Modem control
#define	SC16IS750_LSR		(0X05)		// Line status
#define	SC16IS750_MSR		(0X06)		// Modem status
#define	SC16IS750_SPR		(0X07)		// Scratch pad
#define	SC16IS750_TCR		(0X06)		// Transmission control
#define	SC16IS750_TLR		(0X07)		// Trigger level
#define	SC16IS750_TXLVL		(0X08)		// TX FIFO level
#define	SC16IS750_RXLVL		(0X09)		// RX FIFO level
#define	SC16IS750_IODIR		(0X0A)		// IO pin direction
#define	SC16IS750_IOSTATE	(0X0B)		// IO pin state
#define	SC16IS750_IOINTENA	(0X0C)		// IO interrupt enable
#define	SC16IS750_IOCONTROL	(0X0E)		// IO control
#define	SC16IS750_EFCR		(0X0F)		// Extra features control

#define	SC16IS750_DLL		(0x00)
#define	SC16IS750_DLH		(0X01)

#define	SC16IS750_EFR		(0X02)
#define	SC16IS750_XON1		(0X04)
#define	SC16IS750_XON2		(0X05)
#define	SC16IS750_XOFF1		(0X06)
#define	SC16IS750_XOFF2		(0X07)

#define	SC16IS750_INT_CTS	(0X80)
#define	SC16IS750_INT_RTS	(0X40)
#define	SC16IS750_INT_XOFF	(0X20)
#define	SC16IS750_INT_SLEEP	(0X10)
#define	SC16IS750_INT_MODEM	(0X08)
#define	SC16IS750_INT_LINE	(0X04)
#define	SC16IS750_INT_THR	(0X02)
#define	SC16IS750_INT_RHR	(0X01)

// FCR register bits
#define	FCR_FIFO_ENABLE				(1 << 0)
#define	FCR_FIFO_RX_RESET			(1 << 1)
#define	FCR_FIFO_TX_RESET			(1 << 2)
#define	FCR_FIFO_TX_TRIGGER_LVL_LSB	(1 << 4) 	// Can only be set when EFR[4] is set
#define	FCR_FIFO_TX_TRIGGER_LVL_MSB	(1 << 5)	// --
#define	FCR_FIFO_RX_TRIGGER_LVL_LSB	(1 << 6)
#define	FCR_FIFO_RX_TRIGGER_LVL_MSB	(1 << 7)

// LCR register bits
#define	LCR_WORD_LENGTH0			(1 << 0)
#define	LCR_WORD_LENGTH1			(1 << 1)
#define	LCR_STOP_BITS_15_OR_2		(1 << 2)	// 0 => 1 stop bit, 1 => 1.5 if word length==5, 2 otherwise
#define	LCR_PARITY_ENABLE			(1 << 3)
#define	LCR_PARITY_EVEN				(1 << 4)
#define	LCR_PARITY_FORCE			(1 << 5)
#define	LCR_TX_BREAK				(1 << 6)
#define	LCR_DIVISOR_LATCH_ENABLE	(1 << 7)

#define	LCR_WORD_LENGTH_5			(0x00)
#define	LCR_WORD_LENGTH_6			(0x01)
#define	LCR_WORD_LENGTH_7			(0x02)
#define	LCR_WORD_LENGTH_8			(0x03)

#define	SET_LCR_PARITY_NONE			(0x00)
#define	SET_LCR_PARITY_ODD			((~LCR_PARITY_EVEN) | LCR_PARITY_ENABLE | (~LCR_PARITY_FORCE))
#define	SET_LCR_PARITY_EVEN			(LCR_PARITY_EVEN | LCR_PARITY_ENABLE | (~LCR_PARITY_FORCE))
#define	SET_LCR_PARITY_1			((~LCR_PARITY_EVEN) | LCR_PARITY_ENABLE | LCR_PARITY_FORCE)
#define	SET_LCR_PARITY_0			(LCR_PARITY_EVEN | LCR_PARITY_ENABLE | LCR_PARITY_FORCE)

#define	LCR_ACCESS_EFR				(0xBF)

// LSR register bits
#define	LSR_DATA_AVAILABLE			(1 << 0)
#define	LSR_OVERRUN_ERROR			(1 << 1)
#define	LSR_PARITY_ERROR			(1 << 2)
#define	LSR_FRAMING_ERROR			(1 << 3)
#define	LSR_BREAK_INTERRUPT			(1 << 4)
#define	LSR_THR_EMPTY				(1 << 5)
#define	LSR_TRANSMITTER_EMPTY		(1 << 6)
#define	LSR_FIFO_ERROR				(1 << 7)

// MCR register bits
#define	MCR_nDTR					(1 << 0)	// !DTR
#define	MCR_nRTS					(1 << 1)	// !CTS
#define	MCR_TCR_TLR_ENABLE			(1 << 2)	// Can only be modified when EFR[4] is set
#define	MCR_LOOPBACK_ENABLE			(1 << 4)
#define	MCR_XONANY_ENABLE			(1 << 5)	// Can only be modified when EFR[4] is set
#define	MCR_IRDA_ENABLE				(1 << 6)	// --
#define	MCR_CLOCK_DIVISOR			(1 << 7)	// --

// MSR register bits
#define	MSR_nCTS_CHANGED			(1 << 0)
#define	MSR_nDSR_CHANGED			(1 << 1)
#define	MSR_nRI_CHANGED				(1 << 2)
#define	MSR_nCD_CHANGED				(1 << 3)
#define	MSR_CTS						(1 << 4)
#define	MSR_DSR						(1 << 5)
#define	MSR_RI						(1 << 6)
#define	MSR_CD						(1 << 7)

// IER register bits
#define	IER_RHR_INT_ENABLE			(1 << 0)
#define	IER_THR_INT_ENABLE			(1 << 1)
#define	IER_RX_INT_ENABLE			(1 << 2)
#define	IER_MODEM_STATUS_INT_ENABLE	(1 << 3)
#define	IER_SLEEP_MODE_ENABLE		(1 << 4)
#define	IER_XOFF_INTERRUPT_ENABLE	(1 << 5)
#define	IER_nRTS_INTERRUPT_ENABLE	(1 << 6)
#define	IER_nCTS_INTERRUPT_ENABLE	(1 << 7)

// IIR register bits
#define	IIR_NO_INTERRUPT			(1 << 0)

// EFR register bits
#define	EFR_ENHANCED_FUNC_ENABLE	(1 << 4)
#define	EFR_SPECIAL_CHAR_ENABLE		(1 << 5)
#define	EFR_nRTS_FLOWCONTROL_ENABLE	(1 << 6)
#define	EFR_nCTS_FLOWCONTROL_ENABLE	(1 << 7)

// EFCR register bits
#define	EFCR_RX_DISABLE		(1 << 1)
#define	EFCR_TX_DISABLE		(1 << 2)

#define	SC16IS750_XTAL_FREQ			(14745600UL)
#define	SC16IS750_REG_ADDR_SHIFT	( 3 )

#define	NO_PARITY			( 0 )
#define	ODD_PARITY			( 1 )
#define	EVEN_PARITY			( 2 )
#define	FORCE_1_PARITY		( 3 )
#define	FORCE_0_PARITY		( 4 )

class I2C_SC16IS750
{
	public:
				explicit I2C_SC16IS750( void ) = default;
		bool	begin( uint32_t );
		// flawfinder: ignore
		int8_t	read( void );
		uint8_t	write( uint8_t );
		int		available( void );
		int		peek( void );
		void	flush( void );

		bool	digitalRead( uint8_t );
		bool	digitalWrite( uint8_t, bool );
		bool	pinMode( uint8_t, bool );
		void	set_address( uint8_t );
		bool 	setup_GPIO_interrupt( uint8_t, bool );

	private:

		uint8_t address		= ( DEFAULT_SC16IS750_ADDR >> 1 );
		bool	has_peek	= false;
		int		peek_byte	= -1;

		void	FIFO_enable( bool );
		uint8_t get_IO_state( void );
		void	set_baudrate( uint32_t );
		int8_t	read_register( uint8_t );
		bool	write_register( uint8_t, uint8_t );
		void	set_line( uint8_t, uint8_t, uint8_t );
		int8_t	read_byte( void );
		uint8_t	write_byte( uint8_t );
		bool	available_data_in_FIFO( void );
		void    FIFO_reset( bool, bool );
		bool	test( void );
		void	enable_extra_features( void );
		void 	toggle_sleep( bool );
		void	set_flow_control_levels( uint8_t, uint8_t );
		bool 	set_IO_direction( uint8_t pin, bool write );

		void 	enable_tcr_tlr( void );
		void	enable_TX_RX( void );
};

#endif
