/*
	AstroWeatherStation.ino

	(c) 2023 F.Lesage

	1.0 - Initial version.
	1.1 - Refactored to remove unnecessary global variables
	1.2 - Added experimental SQM feature

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

#include <ESP32Time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include "Adafruit_TSL2591.h"
#include <SoftwareSerial.h>
#include "time.h"

#define REV "1.2.0-20230430"
#define DEBUG_MODE 1

// You must customise weather_config.h
#include "weather_config.h"

// Wind sensors
#define GPIO_WIND_VANE_RX		GPIO_NUM_27	// RO Pin
#define GPIO_WIND_VANE_TX		GPIO_NUM_26	// DI Pin
#define GPIO_WIND_VANE_CTRL		GPIO_NUM_14	// RE+DE Pins

#define GPIO_ANEMOMETER_RX		GPIO_NUM_32	// RO Pin
#define GPIO_ANEMOMETER_TX		GPIO_NUM_33	// DI Pin
#define GPIO_ANEMOMETER_CTRL	GPIO_NUM_25	// RE+DE Pins

#define SEND	HIGH
#define RECV	LOW

#define WIND_VANE_MAX_TRIES		3		// Tests show that if there is no valid answer after 3 attempts, the sensor is not available
#define ANEMOMETER_MAX_TRIES	3		// Tests show that if there is no valid answer after 3 attempts, the sensor is not available

// Rain sensor
#define GPIO_RG9_RX			GPIO_NUM_17	// J2 SO PIN
#define GPIO_RG9_TX			GPIO_NUM_16	// J2 SI PIN
#define GPIO_RG9_MCLR		GPIO_NUM_23	// J2 MCLR PIN
#define GPIO_RG9_RAIN		GPIO_NUM_35	// J1 OUT PIN
#define RG9_SERIAL_SPEEDS	7
#define RG9_PROBE_RETRIES	5
#define RG9_OK				0
#define RG9_FAIL			127
#define RG9_UART			2

// Relays
#define GPIO_RELAY_3_3V		GPIO_NUM_5
#define GPIO_RELAY_12V		GPIO_NUM_18

// Battery level
#define GPIO_BAT_ADC			GPIO_NUM_13
#define GPIO_BAT_ADC_EN			GPIO_NUM_12
#define LOW_BATTERY_COUNT_MIN	5
#define LOW_BATTERY_COUNT_MAX	10
#define BAT_V_MAX				4200	// in mV
#define BAT_V_MIN				3000	// in mV
#define BAT_LEVEL_MIN			33		// in %, corresponds to ~3.4V for a typical Li-ion battery
#define VCC						3300	// in mV
#define V_DIV_R1				82000	// voltage divider R1 in ohms
#define V_DIV_R2				300000	// voltage divider R2 in ohms
#define ADC_MAX					4096	// 12 bits resolution
#define V_MAX_IN				( BAT_V_MAX*V_DIV_R2 )/( V_DIV_R1+V_DIV_R2 )	// in mV
#define V_MIN_IN				( BAT_V_MIN*V_DIV_R2 )/( V_DIV_R1+V_DIV_R2 )	// in mV
#define ADC_V_MAX				( V_MAX_IN*ADC_MAX / VCC )
#define ADC_V_MIN				( V_MIN_IN*ADC_MAX / VCC )

// SQM
#define	DOWN	-1
#define	UP		1
#define	MSAS_CALIBRATION_OFFSET	-0.55F	// Taken from a comparison with a calibrated SQM-L(E/U)

// Misc
#define GPIO_DEBUG		GPIO_NUM_34
#define US_SLEEP		5 * 60 * 1000000	// 5 minutes

#define MLX_SENSOR			0x01
#define TSL_SENSOR			0x02
#define BME_SENSOR			0x04
#define WIND_VANE_SENSOR	0x08
#define ANEMOMETER_SENSOR	0x10
#define RG9_SENSOR			0x20
#define ALL_SENSORS			( MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RG9_SENSOR )

RTC_DATA_ATTR byte reboot_count;
RTC_DATA_ATTR byte prev_available_sensors;
RTC_DATA_ATTR byte low_battery_event_count;

void setup()
{
	float	battery_level = 0.0;
	byte	ntp_retries = 5,
			ntp_synced = 0,
			rain_intensity,
			rain_event,
			available_sensors = 0,
			debug_mode;
	char	string[64],
			wakeup_string[50];

	struct tm timeinfo;

	const char	*ntp_server = "pool.ntp.org";
	const char	*tz_info	= TZNAME;

	ESP32Time rtc( 0 );

	Adafruit_BME280		bme;
	Adafruit_MLX90614	mlx = Adafruit_MLX90614();
	Adafruit_TSL2591	tsl = Adafruit_TSL2591( 2591 );

	SoftwareSerial		anemometer( GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX );
	SoftwareSerial		wind_vane( GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX );

	HardwareSerial		rg9( RG9_UART );

	StaticJsonDocument<384> values;

	esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
	wakeup_reason_to_string( wakeup_reason, wakeup_string );

	if ( wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 )	// It may not be a power issue but it is definitely not the timer or a rain event
		reboot_count++;

	Serial.begin( 115200 );
	delay( 100 );

	pinMode( GPIO_DEBUG, INPUT );
	debug_mode = static_cast<byte>( 1-gpio_get_level( GPIO_DEBUG ) ) | DEBUG_MODE;

	if ( debug_mode )
		displayBanner( wakeup_string );

	pinMode( GPIO_BAT_ADC_EN, OUTPUT );

	pinMode( GPIO_RELAY_3_3V, OUTPUT );
	pinMode( GPIO_RELAY_12V, OUTPUT );
	digitalWrite( GPIO_RELAY_3_3V, LOW );
	digitalWrite( GPIO_RELAY_12V, LOW );

	battery_level = get_battery_level( debug_mode );
	values[ "battery_level" ] = battery_level;

	if ( connect_to_wifi( debug_mode ) ) {

		rain_event = ( ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason );
		configTzTime( tz_info, ntp_server );

		while ( !( ntp_synced = getLocalTime( &timeinfo )) && ( --ntp_retries > 0 ) ) {

			delay( 1000 );
			configTzTime( tz_info, ntp_server );
		}

		if ( debug_mode ) {

			Serial.print( "Reboot counter=" );
			Serial.println( reboot_count );
			Serial.printf( "%sNTP Synchronised. ", ntp_synced?"":"NOT " );
			Serial.print( "Time and date: " );
			Serial.println( &timeinfo, "%Y-%m-%d %H:%M:%S" );
		}

		initialise_sensors( &bme, &mlx, &tsl, &anemometer, &wind_vane, &rg9, &available_sensors, reboot_count, debug_mode );
		retrieve_sensor_data( values, &bme, &mlx, &tsl, &anemometer, &wind_vane, &rg9, ntp_synced, rain_event, &rain_intensity, debug_mode, &available_sensors );

		if ( rain_event ) {

			if ( rain_intensity > 0 )	// Avoid false positives
				send_rain_event_alarm( rain_intensity, debug_mode );

			else {

				if ( debug_mode )
					Serial.println( "Rain event false positive, back to bed." );

				goto enter_sleep;	// The hell with bigots and their aversion to goto's
			}
		}

		if ( battery_level <= BAT_LEVEL_MIN ) {

			memset( string, 0, 64 );
			snprintf( string, 25, "Battery level = %03.2f%%\n", battery_level );

			// Deal with ADC output accuracy and no need to stress about it, a few warnings are enough
			if (( low_battery_event_count++ >= LOW_BATTERY_COUNT_MIN ) && ( low_battery_event_count++ <= LOW_BATTERY_COUNT_MAX ))
				send_alarm( "Low battery", string, debug_mode );

			if ( debug_mode )
				Serial.printf( "%sLow battery event count = %d\n", string, low_battery_event_count );

		} else
			low_battery_event_count = 0;

		send_data( values, debug_mode );

		if ( prev_available_sensors != available_sensors ) {

			prev_available_sensors = available_sensors;
			report_unavailable_sensors( available_sensors, debug_mode );
		}
	}

enter_sleep:

	digitalWrite( GPIO_RELAY_3_3V, HIGH );
	digitalWrite( GPIO_RELAY_12V, HIGH );

	if ( debug_mode )
		Serial.println( "Entering sleep mode." );

	esp_sleep_enable_timer_wakeup( US_SLEEP );
	esp_sleep_pd_config( ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF );
	esp_sleep_enable_ext0_wakeup( GPIO_RG9_RAIN, LOW );
	esp_deep_sleep_start();
}

void loop()
{
	send_alarm( "Bug", "Entered the loop function.", 0 );
	Serial.println( "BUG! Entered the loop function." );
	delay( 5000 );
}

/*
   --------------------------------

    Data handling

   --------------------------------
*/
void retrieve_sensor_data( JsonDocument& values, Adafruit_BME280 *bme, Adafruit_MLX90614 *mlx, Adafruit_TSL2591 *tsl, SoftwareSerial *anemometer, SoftwareSerial *wind_vane, HardwareSerial *rg9, byte is_ntp_synced, byte rain_event, byte *rain_intensity, byte debug_mode, byte *available_sensors )
{
	float		temp,
				pres,
				rh,
				msas = -1,
				nelm,
				ambient_temp = 0,
				sky_temp = 0;

	uint16_t	ch0,
				ch1,
				gain,
				int_time;

	time_t		now;

	if ( is_ntp_synced )
		time( &now );

	values[ "timestamp" ] = ( is_ntp_synced ? now : 0 ); // Give a chance to the server to insert its own timestamp
	values[ "rain_event" ] = rain_event;

	read_BME( bme, &temp, &pres, &rh, *available_sensors, debug_mode );
	values[ "temp" ] = temp;
	values[ "pres" ] = pres;
	values[ "rh" ] = rh;

	values[ "lux" ] = read_TSL( tsl, *available_sensors, debug_mode );

	read_MLX( mlx, &ambient_temp, &sky_temp, *available_sensors, debug_mode );
	values[ "ambient_temp" ] = ambient_temp;
	values[ "sky_temp" ] = sky_temp;

	values[ "wind_direction" ] = read_wind_vane( wind_vane, available_sensors, debug_mode );
	values[ "wind_speed" ] = read_anemometer( anemometer, available_sensors, debug_mode );

	*rain_intensity = read_RG9( rg9, debug_mode );
	values[ "rain_intensity" ] = *rain_intensity;

	read_SQM( tsl, *available_sensors, debug_mode, ambient_temp, &msas, &nelm, &ch0, &ch1, &gain, &int_time );
	values[ "msas" ] = msas;
	values[ "nelm" ] = nelm;
	values[ "gain" ] = gain;
	values[ "int_time" ] = int_time;
	values[ "ch0" ] = ch0;
	values[ "ch1" ] = ch1;
 
	values[ "sensors" ] = *available_sensors;
}

/*
   --------------------------------

    Sensor intialisation

   --------------------------------
*/
void initialise_sensors( Adafruit_BME280 *bme, Adafruit_MLX90614 *mlx, Adafruit_TSL2591 *tsl, SoftwareSerial *anemometer, SoftwareSerial *wind_vane, HardwareSerial *rg9, byte *available_sensors, byte reboot_count, byte debug_mode )
{
	initialise_BME( bme, available_sensors, debug_mode );
	initialise_MLX( mlx, available_sensors, debug_mode );
	initialise_TSL( tsl, available_sensors, debug_mode );
	initialise_anemometer( anemometer );
	initialise_wind_vane( wind_vane );

	switch ( initialise_RG9( rg9, available_sensors, reboot_count, debug_mode ) ) {
		case RG9_OK:
		case RG9_FAIL:
			break;
		case 'B':
			send_alarm( "RG9 low voltage", "", debug_mode );
			break;
		case 'O':
			send_alarm( "RG9 problem", "Reset because of stack overflow, report problem to support.", debug_mode );
			break;
		case 'U':
			send_alarm( "RG9 problem", "Reset because of stack underflow, report problem to support.", debug_mode );
			break;
		default:
			break;
	}
}

void initialise_anemometer( SoftwareSerial *anemometer )
{
	// TODO: Test ESP32's specific RS485 mode
	anemometer->begin( 9600 );
	pinMode( GPIO_ANEMOMETER_CTRL, OUTPUT );
}

void initialise_BME( Adafruit_BME280 *bme, byte *available_sensors, byte debug_mode )
{
	if ( !bme->begin( 0x76 ) ) {

		if ( debug_mode )
			Serial.println( "Could not find BME280." );

	} else {

		if ( debug_mode )
			Serial.println( "Found BME280." );

		*available_sensors |= BME_SENSOR;
	}
}

void initialise_MLX( Adafruit_MLX90614 *mlx, byte *available_sensors, byte debug_mode  )
{
	*mlx = Adafruit_MLX90614();

	if ( !mlx->begin() ) {

		if ( debug_mode )
			Serial.println( "Could not find MLX90614" );

	} else {

		if ( debug_mode )
			Serial.println( "Found MLX90614" );

		*available_sensors |= MLX_SENSOR;
	}
}

char initialise_RG9( HardwareSerial *rg9, byte *available_sensors, byte reboot_count, byte debug_mode )
{
	const int	bps[ RG9_SERIAL_SPEEDS ] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600 };
	char		str[ 128 ];
	byte		l;
	
	if ( reboot_count == 1 )    // Give time to the sensor to initialise before trying to probe it
		delay( 5000 );

	if ( debug_mode )
		Serial.printf( "Probing RG9 ... " );

	for ( byte i = 0; i < RG9_PROBE_RETRIES; i++ )
		for ( byte j = 0; j < RG9_SERIAL_SPEEDS; j++ ) {

			rg9->begin( bps[j], SERIAL_8N1, GPIO_RG9_RX, GPIO_RG9_TX );
			rg9->println();
      		rg9->println();

			if ( debug_mode ) {
				Serial.printf( " @ %dbps: got [", bps[j] );
				Serial.flush();
			}

			rg9->println( "B" );
			l = RG9_read_string( rg9, str, 128 );

			if ( debug_mode )
				Serial.printf( "%s] ", str );

			if ( !strncmp( str, "Baud ", 5 )) {

				if ( debug_mode ) {
					char bitrate[8];
					strncpy( bitrate, str+5, l-5 );
					Serial.printf( "\nFound RG9 @ %s bps\n", bitrate );
				}

				*available_sensors |= RG9_SENSOR;
				return RG9_OK;
			}

			if ( !strncmp( str, "Reset " , 6 )) {

				char reset_cause = str[6];
				if ( debug_mode )
					Serial.printf( "\nFound G9 @ %dbps after it was reset because of '%s'\nBoot message:\n", bps[j], RG9_reset_cause( reset_cause ));

				while (( l = RG9_read_string( rg9, str, 128 )))
					Serial.println( str );

				*available_sensors |= RG9_SENSOR;
				return reset_cause;
			}

			rg9->end();
		}

	if ( debug_mode )
		Serial.println( "Could not find RG9, resetting..." );

	pinMode( GPIO_RG9_MCLR, OUTPUT );
	digitalWrite( GPIO_RG9_MCLR, LOW );
	delay( 500 );
	digitalWrite( GPIO_RG9_MCLR, HIGH );

	return RG9_FAIL;
}

void initialise_TSL( Adafruit_TSL2591 *tsl, byte *available_sensors, byte debug_mode )
{
	if ( !tsl->begin() ) {

		if ( debug_mode )
			Serial.println( "Could not find TSL2591" );

	} else {

		if ( debug_mode )
			Serial.println( "Found TSL2591" );

		tsl->setGain( TSL2591_GAIN_LOW );
		tsl->setTiming( TSL2591_INTEGRATIONTIME_100MS );
		*available_sensors |= TSL_SENSOR;
	}
}

void initialise_wind_vane( SoftwareSerial *wind_vane )
{
	// TODO: Test ESP32's specific RS485 mode
	wind_vane->begin( 9600 );
	pinMode( GPIO_WIND_VANE_CTRL, OUTPUT );
}

/*
   --------------------------------

    Utility functions

   --------------------------------
*/

//
// Formulas inferred from TSL2591 datasheet fig.15, page 9. Data points graphically extracted (yuck!).
// Thanks to Marco Gulino <marco.gulino@gmail.com> for pointing this graph to me!
// I favoured a power function over an affine because it better followed the points on the graph
// It may however be overkill since the points come from a PDF graph: the source error is certainly not negligible :-)
//
float ch0_temperature_factor( float temp )
{
	return 0.9759F + 0.00192947F*pow( temp, 0.783129F );
}

float ch1_temperature_factor( float temp )
{
	return 1.05118F - 0.0023342F*pow( temp, 0.958056F );
}

void change_gain( Adafruit_TSL2591 *tsl, byte upDown,  tsl2591Gain_t *gain_idx )
{
	tsl2591Gain_t	g = static_cast<tsl2591Gain_t>( *gain_idx + 16*upDown );

	if ( g < TSL2591_GAIN_LOW )
		g = TSL2591_GAIN_LOW;
	else if ( g > TSL2591_GAIN_MAX )
		g = TSL2591_GAIN_MAX;

	if ( g != *gain_idx ) {

		tsl->setGain( g );
		*gain_idx = g;
	}	
}

void change_integration_time( Adafruit_TSL2591 *tsl, byte upDown, tsl2591IntegrationTime_t *int_time_idx )
{
	tsl2591IntegrationTime_t	t = static_cast<tsl2591IntegrationTime_t>( *int_time_idx + 2*upDown );

	if ( t < TSL2591_INTEGRATIONTIME_100MS )
		t = TSL2591_INTEGRATIONTIME_100MS;
	else if ( t > TSL2591_INTEGRATIONTIME_600MS )
		t = TSL2591_INTEGRATIONTIME_600MS;

	if ( t != *int_time_idx ) {

		tsl->setTiming( t );
		*int_time_idx = t;
	}	
}

byte connect_to_wifi( byte debug_mode )
{
	const char	*ssid				= SSID;
	const char	*password			= PASSWORD;
	byte		remaining_attempts	= 10;

	if ( debug_mode )
		Serial.printf( "Attempting to connect to SSID [%s] ", ssid );

	WiFi.begin( ssid, password );

	while (( WiFi.status() != WL_CONNECTED ) && ( --remaining_attempts > 0 )) {

		if ( debug_mode )
			Serial.print( "." );
		delay( 1000 );
	}

	if ( WiFi.status () == WL_CONNECTED ) {

		if ( debug_mode )
			Serial.printf( " OK. Using IP [%s]\n", WiFi.localIP().toString().c_str() );
		return 1;
	}

	if ( debug_mode )
		Serial.println( "NOK." );

	return 0;
}

void displayBanner( char *wakeup_string )
{
	char	string[64];
	byte	i;

	delay(2000);
	Serial.println( "\n##############################################################" );
	Serial.println( "# AstroWeatherStation                                        #" );
	Serial.println( "#  (c) Lesage Franck - lesage@datamancers.net                #" );
	snprintf( string, 64, "# Build %-52s #\n", REV );
	Serial.printf( string );
	Serial.println( "#------------------------------------------------------------#" );
	snprintf( string, 64, "# %-58s #\n", wakeup_string );
	Serial.printf( string );
	Serial.println( "#------------------------------------------------------------#" );
	Serial.println( "# GPIO PIN CONFIGURATION                                     #" );
	Serial.println( "#------------------------------------------------------------#" );
	memset( string, 0, 64 );
	snprintf( string, 61, "# Wind vane : RX=%d TX=%d CTRL=%d", GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX, GPIO_WIND_VANE_CTRL );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
  	memset( string, 0, 64 );
	snprintf( string, 61, "# Anemometer: RX=%d TX=%d CTRL=%d", GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX, GPIO_ANEMOMETER_CTRL );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
	memset( string, 0, 64 );
	snprintf( string, 61, "# RG9       : RX=%d TX=%d MCLR=%d RAIN=%d", GPIO_RG9_RX, GPIO_RG9_TX, GPIO_RG9_MCLR, GPIO_RG9_RAIN );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
	memset( string, 0, 64 );
	snprintf( string, 61, "# 3.3V RELAY: %d", GPIO_RELAY_3_3V );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
	memset( string, 0, 64 );
	snprintf( string, 61, "# 12V RELAY : %d", GPIO_RELAY_12V );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
	memset( string, 0, 64 );
	snprintf( string, 61, "# DEBUG     : %d", GPIO_DEBUG );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
	memset( string, 0, 64 );
	snprintf( string, 61, "# BAT LVL   : SW=%d ADC=%d", GPIO_BAT_ADC_EN, GPIO_BAT_ADC );
	for ( i = strlen( string ); i < 61; string[i++] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
	Serial.println( "##############################################################" );
}

void post_content( const char *endpoint, const char *jsonString, byte debug_mode )
{
  	const char *server = SERVER;
	const char *url = URL;
	const char *root_ca = ROOT_CA;
	WiFiClientSecure client;
	HTTPClient http;
	byte status;
	char *final_endpoint;

	if ( debug_mode )
		Serial.print( "Connecting to server..." );

	client.setCACert( root_ca );

	if (!client.connect( server, 443 )) {

		if ( debug_mode )
			Serial.println( "NOK." );

	} else {

		if ( debug_mode )
			Serial.println( "OK." );

		final_endpoint = (char *)malloc( strlen( url ) + strlen( endpoint ) + 2 );
		strcpy( final_endpoint, url );
		strcat( final_endpoint, endpoint );
		http.begin( client, final_endpoint );
		http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
		http.addHeader( "Content-Type", "application/json" );
		status = http.POST( jsonString );

		if ( debug_mode ) {
			Serial.println();
			Serial.print( "HTTP response: " );
			Serial.println( status );
		}

		client.stop();
		free( final_endpoint );
	}
}

void report_unavailable_sensors( byte available_sensors, byte debug_mode )
{
	const char	sensor_name[6][12] = {"MLX96014 ", "TSL2591 ", "BME280 ", "WIND VANE ", "ANEMOMETER ", "RG9 "};
	char		unavailable_sensors[96] = "Unavailable sensors: ";
	byte 		j = available_sensors;

	for ( byte i = 0; i < 6; i++ ) {
		if ( !( available_sensors & 1 ))
			strncat( unavailable_sensors, sensor_name[i], 12 );
		available_sensors >>= 1;
	}

	if ( debug_mode ) {
		Serial.print( unavailable_sensors );
		if ( j == ALL_SENSORS )
			Serial.println( "none" );
	}

	if ( j != ALL_SENSORS )
		send_alarm( "Unavailable sensors report", unavailable_sensors, debug_mode );
}

byte RG9_read_string( HardwareSerial *rg9, char *str, byte len )
{
	byte i;
	
	delay( 500 );

	if ( rg9->available() > 0 ) {

		i = rg9->readBytes( str, len );
		str[ i-2 ] = 0;	// trim trailing \n
		return i-1;
	}
	return 0;
}

const char *RG9_reset_cause( char code )
{
	switch ( code ) {
		case 'N': return "Normal power up";
		case 'M': return "MCLR";
		case 'W': return "Watchdog timer reset";
		case 'O': return "Start overflow";
		case 'U': return "Start underflow";
		case 'B': return "Low voltage";
		case 'D': return "Other";
		default: return "Unknown";
	}
}

void send_data( const JsonDocument& values, byte debug_mode )
{
	char json[400];
	serializeJson( values, json );

	if ( debug_mode ) {
		Serial.print( "Sending JSON: " );
		serializeJson( values, Serial );
		Serial.println();
	}

	post_content( "newData.php", json, debug_mode );
}

void send_alarm( const char *subject, const char *message, byte debug_mode )
{
	DynamicJsonDocument content( 512 );
	char jsonString[ 600 ];
	content["subject"] = subject;
	content["message"] = message;

	serializeJson( content, jsonString );
	post_content( "alarm.php", jsonString, debug_mode );
}

void send_rain_event_alarm( byte rain_intensity, byte debug_mode )
{
	const char	intensity_str[7][13] = { "Rain drops", "Very light", "Medium light", "Medium", "Medium heavy", "Heavy", "Violent" };  // As described in RG-9 protocol
	char		msg[32];

	snprintf( msg, 32, "RAIN! Level = %s", intensity_str[ rain_intensity ] );
	send_alarm( "Rain event", msg, debug_mode );
}

void wakeup_reason_to_string( esp_sleep_wakeup_cause_t wakeup_reason, char *wakeup_string )
{
	switch ( wakeup_reason ) {
		case ESP_SLEEP_WAKEUP_EXT0 :
			snprintf( wakeup_string, 49, "Awakened by RTC IO: Rain event!" );
			break;
		case ESP_SLEEP_WAKEUP_TIMER :
			snprintf( wakeup_string, 49, "Awakened by timer" );
			break;
		default :
			snprintf( wakeup_string, 49, "Awakened by other: %d", wakeup_reason );
			break;
	}
}

/*
   --------------------------------

    Sensor reading

   --------------------------------
*/
float get_battery_level( byte debug_mode )
{
	int		adc_value = 0;
	float	battery_level,
			adc_v_in,
			bat_v;
	
	if ( debug_mode )
		Serial.print( "Battery level: " );

	digitalWrite( GPIO_BAT_ADC_EN, HIGH );
	delay( 500 );
	for( byte i = 0; i < 5; i++ ) {

		adc_value += analogRead( GPIO_BAT_ADC );
		delay( 100 );
	}
	adc_value = static_cast<int>( adc_value / 5 );
	
	battery_level = map( adc_value, ADC_V_MIN, ADC_V_MAX, 0, 100 );

	if ( debug_mode ) {

		adc_v_in = static_cast<float>(adc_value) * VCC / ADC_V_MAX;
		bat_v = adc_v_in * ( V_DIV_R1 + V_DIV_R2 ) / V_DIV_R2;
		Serial.printf( "%03.2f%% (ADC value=%d, ADC voltage=%1.3fV, battery voltage=%1.3fV)\n", battery_level, adc_value, adc_v_in / 1000.F, bat_v / 1000.F );

	}

	digitalWrite( GPIO_BAT_ADC_EN, LOW );
	return battery_level;
}

float read_anemometer( SoftwareSerial *anemometer, byte *available_sensors, byte debug_mode )
{
	const byte	anemometer_request[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0a };
	byte		answer[7],
				i = 0,
				j;
	float		wind_speed = -1.F;

	answer[1] = 0;

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_ANEMOMETER_CTRL, SEND );
		anemometer->write( anemometer_request, 8 );
		anemometer->flush();

		digitalWrite( GPIO_ANEMOMETER_CTRL, RECV );
		anemometer->readBytes( answer, 7 );

		if ( debug_mode ) {

			Serial.print( "Anemometer answer: " );
			for ( j = 0; j < 6; j++ )
				Serial.printf( "%02x ", answer[j] );

		}

		if ( answer[1] == 0x03 ) {

			wind_speed = static_cast<float>(answer[4]) / 10.F;

			if ( debug_mode )
				Serial.printf( "\nWind speed: %02.2f m/s\n", wind_speed );

		} else {

			if ( debug_mode )
				Serial.println( "(Error)." );
			delay( 500 );
		}

		if ( ++i == ANEMOMETER_MAX_TRIES ) {

			*available_sensors &= ~ANEMOMETER_SENSOR;
			wind_speed = -1.F;
			return wind_speed;
		}
	}

	*available_sensors |= ANEMOMETER_SENSOR;
	return wind_speed;
}

void read_BME( Adafruit_BME280 *bme, float *temp, float *pres, float *rh, byte available_sensors, byte debug_mode )
{
	if ( ( available_sensors & BME_SENSOR ) == BME_SENSOR ) {

		*temp = bme->readTemperature();
		*pres = bme->readPressure() / 100.F;
		*rh = bme->readHumidity();

		if ( debug_mode ) {
			Serial.printf( "Temperature = %2.2f °C\n", *temp  );
			Serial.printf( "Pressure = %4.2f hPa\n", *pres );
			Serial.printf( "RH = %3.2f %%\n", *rh );
		}
		return;
	}

	*temp = -99.F;
	*pres = 0.F;
	*rh = -1.F;
}

void read_MLX( Adafruit_MLX90614 *mlx, float *ambient_temp, float *sky_temp, byte available_sensors, byte debug_mode  )
{
	if ( ( available_sensors & MLX_SENSOR ) == MLX_SENSOR ) {

		*ambient_temp = mlx->readAmbientTempC();
		*sky_temp = mlx->readObjectTempC();

		if ( debug_mode ) {
			Serial.print( "AMB = " );
			Serial.print( *ambient_temp );
			Serial.print( "°C SKY = " );
			Serial.print( *sky_temp );
			Serial.println( "°C" );
		}
		return;
	}
	*ambient_temp = -99.F;
	*sky_temp = -99.F;
}

byte read_RG9( HardwareSerial *rg9, byte debug_mode )
{
	char	msg[128];
	
	rg9->println( "R" );
	memset( msg, 0, 128 );
	RG9_read_string( rg9, msg, 128 );

	if ( debug_mode )
		Serial.printf( "RG9 STATUS = [%s]\n", msg );

	return static_cast<byte>( msg[2] - '0' );
}

void read_SQM( Adafruit_TSL2591 *tsl, byte available_sensors, byte debug_mode, float ambient_temp, float *msas, float *nelm, uint16_t *ch0, uint16_t *ch1, uint16_t *gain, uint16_t *int_time )
{
	tsl->setGain( TSL2591_GAIN_LOW );
	tsl->setTiming( TSL2591_INTEGRATIONTIME_100MS );
		
	while ( !SQM_get_msas_nelm( tsl, debug_mode, ambient_temp, msas, nelm, ch0, ch1, gain, int_time ));
}

byte SQM_get_msas_nelm( Adafruit_TSL2591 *tsl, byte debug_mode, float ambient_temp, float *msas, float *nelm, uint16_t *ch0, uint16_t *ch1, uint16_t *gain, uint16_t *int_time )
{
	uint32_t	lum;
	uint16_t	ir,
				full,
				visible;
	byte		iterations = 1;

	tsl2591Gain_t				gain_idx;
	tsl2591IntegrationTime_t 	int_time_idx;

	const uint16_t	gain_factor[4]		= { 1, 25, 428, 9876 },
					integration_time[6]	= { 100, 200, 300, 400, 500, 600 };

	gain_idx = tsl->getGain();
	int_time_idx = tsl->getTiming();
	lum = tsl->getFullLuminosity();
	ir = lum >> 16;
	full = lum & 0xFFFF;
	ir = static_cast<uint16_t>( static_cast<float>(ir) * ch1_temperature_factor( ambient_temp ) );
	full = static_cast<uint16_t>( static_cast<float>(full) * ch0_temperature_factor( ambient_temp ) );
	visible = full - ir;

	// On some occasions this can happen, leading to high values of "visible" although it is dark (as the variable is unsigned), giving erroneous msas
	if ( full < ir )
		return 0;

	if ( debug_mode )
		Serial.printf( "SQM: gain=0x%02x (%dx) integ=0x%02x (%dms)/ temp=%f / ir=%d full=%d vis=%d\n", gain_idx, gain_factor[ gain_idx >> 4 ], int_time_idx, integration_time[ int_time_idx ], ambient_temp, ir, full, visible );

	// Auto gain and integration time, increase time before gain to avoid increasing noise if we can help it, decrease gain first for the same reason
	if ( visible < 128 ) {

		if ( int_time_idx != TSL2591_INTEGRATIONTIME_600MS ) {

				if ( debug_mode )
					Serial.println( "SQM: Increasing integration time." );

				change_integration_time( tsl, UP, &int_time_idx );
        		return 0;
		}

		if ( gain_idx == TSL2591_GAIN_MAX ) {

			if ( int_time_idx != TSL2591_INTEGRATIONTIME_600MS ) {

				if ( debug_mode )
					Serial.println( "SQM: Increasing integration time." );

				change_integration_time( tsl, UP, &int_time_idx );
        		return 0;

			} else {

				iterations = SQM_read_with_extended_integration_time( tsl, debug_mode, ambient_temp, &ir, &full, &visible );
				if ( full < ir )
					return 0;
			}

		} else {

				if ( debug_mode )
					Serial.println( "Increasing gain." );

				change_gain( tsl, UP, &gain_idx );
        		return 0;
		}

	} else if (( full == 0xFFFF ) || ( ir == 0xFFFF )) {

		if ( gain_idx != TSL2591_GAIN_LOW ) {

			if ( debug_mode )
				Serial.printf( "Decreasing gain." );

			change_gain( tsl, DOWN, &gain_idx );
			return 0;

		} else {

			if ( debug_mode )
				Serial.println( "Decreasing integration time." );

			change_integration_time( tsl, DOWN, &int_time_idx );
			return 0;
		}
	}

	// Comes from Adafruit TSL2591 driver
	float cpl = static_cast<float>( gain_factor[ gain_idx >> 4 ] ) * static_cast<float>( integration_time[ int_time_idx ] ) / 408.F;
	float lux = ( static_cast<float>(visible) * ( 1.F-( static_cast<float>(ir)/static_cast<float>(full))) ) / cpl;

	// About the MSAS formula, quoting http://unihedron.com/projects/darksky/magconv.php:
	// This formula was derived from conversations on the Yahoo-groups darksky-list
	// Topic: [DSLF]  Conversion from mg/arcsec^2 to cd/m^2
	// Date range: Fri, 1 Jul 2005 17:36:41 +0900 to Fri, 15 Jul 2005 08:17:52 -0400
	
	// I added a calibration offset to match readings from my SQM-LE
	*msas = ( log10( lux / 108000.F ) / -0.4F ) + MSAS_CALIBRATION_OFFSET;
	if ( *msas < 0 )
		*msas = 0;
	*nelm = 7.93F - 5.F * log10( pow( 10, ( 4.316F - ( *msas / 5.F ))) + 1.F );
	
	*int_time = integration_time[ int_time_idx ];
	*gain = gain_factor[ gain_idx >> 4 ];
	*ch0 = full;
	*ch1 = ir;
	if ( debug_mode )
		Serial.printf("GAIN=[0x%02hhx/%ux] TIME=[0x%02hhx/%ums] iter=[%d] VIS=[%d] IR=[%d] MPSAS=[%f] NELM=[%f]\n", gain_idx, gain_factor[ gain_idx >> 4 ], int_time_idx, integration_time[ int_time_idx ], iterations, visible, ir, *msas, *nelm );

	return 1;
}

int read_TSL( Adafruit_TSL2591 *tsl, byte available_sensors, byte debug_mode )
{
	int			lux = -1;
	uint32_t	lum;
	uint16_t	ir,
				full;

	if ( ( available_sensors & TSL_SENSOR ) == TSL_SENSOR ) {

		lum = tsl->getFullLuminosity();
		ir = lum >> 16;
		full = lum & 0xFFFF;
		lux = tsl->calculateLux( full, ir );

		if ( debug_mode )
			Serial.printf( "IR=%d FULL=%d VIS=%d Lux = %05d\n", ir, full, full - ir, lux );
	}
	return lux;
}

int read_wind_vane( SoftwareSerial *wind_vane, byte *available_sensors, byte debug_mode )
{
	const byte	wind_vane_request[] = { 0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xc4, 0x38 };
	byte		answer[7],
				i = 0,
				j;
	int			wind_direction = -1;

	answer[1] = 0;

	while ( answer[1] != 0x03 ) {

		digitalWrite( GPIO_WIND_VANE_CTRL, SEND );
		wind_vane->write( wind_vane_request, 8 );
		wind_vane->flush();

		digitalWrite( GPIO_WIND_VANE_CTRL, RECV );
		wind_vane->readBytes( answer, 7 );

		if ( debug_mode ) {

			Serial.print( "Wind vane answer : " );
			for ( j = 0; j < 6; j++ )
				Serial.printf( "%02x ", answer[j] );
		}

		if ( answer[1] == 0x03 ) {

			wind_direction = ( answer[5] << 8 ) + answer[6];

			if ( debug_mode )
				Serial.printf( "\nWind direction: %d°\n", wind_direction );

		} else {

			if ( debug_mode )
				Serial.println( "(Error)." );
			delay( 500 );
		}

		if ( ++i == WIND_VANE_MAX_TRIES ) {

			*available_sensors &= ~WIND_VANE_SENSOR;
			wind_direction = -1;
			return wind_direction;
		}
	}

	*available_sensors |= WIND_VANE_SENSOR;
	return wind_direction;
}

byte SQM_read_with_extended_integration_time( Adafruit_TSL2591 *tsl, byte debug_mode, float ambient_temp, uint16_t *cumulated_ir, uint16_t *cumulated_full, uint16_t *cumulated_visible )
{
	uint32_t	lum;
	uint16_t	ir,
				full;
	byte		iterations = 1;

	while (( *cumulated_visible < 128 ) && ( iterations <= 32 )) {

		iterations++;
		lum = tsl->getFullLuminosity();
		ir = lum >> 16;
		full = lum & 0xFFFF;
		ir = static_cast<uint16_t>( static_cast<float>(ir) * ch1_temperature_factor( ambient_temp ));
		full = static_cast<uint16_t>( static_cast<float>(full) * ch0_temperature_factor( ambient_temp ));
		*cumulated_full += full;
		*cumulated_ir += ir;
		*cumulated_visible = *cumulated_full - *cumulated_ir;

		delay( 50 );
	}

	return iterations;
}
