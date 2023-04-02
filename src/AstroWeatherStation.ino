/*
   AstroWeatherStation.ino

    (c) 2023 F.Lesage

    1.0 - Initial version.
	1.1 - Refactored to remove unnecessary global variables
	
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

#define REV "1.1.0-20230401"
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
#define V_DIV_R1				100000	// voltage divider R1 in ohms
#define V_DIV_R2				300000	// voltage divider R2 in ohms
#define ADC_MAX					4096	// 12 bits resolution
#define V_MAX_IN				(BAT_V_MAX * V_DIV_R2)/(V_DIV_R1 + V_DIV_R2)	// in mV
#define V_MIN_IN				(BAT_V_MIN * V_DIV_R2)/(V_DIV_R1 + V_DIV_R2)	// in mV
#define ADC_V_MAX				(V_MAX_IN * ADC_MAX / VCC)
#define ADC_V_MIN				(V_MIN_IN * ADC_MAX / VCC)

#define GPIO_DEBUG		GPIO_NUM_34
#define US_SLEEP		5 * 60 * 1000000	// 5 minutes

#define MLX_SENSOR			0x01
#define TSL_SENSOR			0x02
#define BME_SENSOR			0x04
#define WIND_VANE_SENSOR	0x08
#define ANEMOMETER_SENSOR	0x10
#define RG9_SENSOR			0x20
#define ALL_SENSORS			(MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RG9_SENSOR)

RTC_DATA_ATTR byte reboot_count;
RTC_DATA_ATTR byte prev_available_sensors;
RTC_DATA_ATTR byte low_battery_event_count;

void setup()
{
	float   battery_level = 0.0;
	byte    ntp_retries = 5,
			ntp_synced = 0,
			rain_intensity,
			rain_event,
			available_sensors = 0,
			debug_mode;
	char	string[64],
			wakeup_string[50];
	struct 	tm timeinfo;

	const char	*ntp_server = "pool.ntp.org";
	const char	*tz_info	= TZNAME;

	ESP32Time rtc( 0 );

	Adafruit_BME280		bme;
	Adafruit_MLX90614	mlx = Adafruit_MLX90614();
	Adafruit_TSL2591	tsl = Adafruit_TSL2591( 2591 );

	SoftwareSerial		anemometer( GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX );
	SoftwareSerial		wind_vane( GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX );

	HardwareSerial		rg9( RG9_UART );

	StaticJsonDocument<256> values;

	esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
	wakeup_reason_to_string( wakeup_reason, wakeup_string );

	if ( wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 )	// It may not be a power issue but it is definitely not the timer or a rain event
		reboot_count++;

	Serial.begin( 115200 );
	delay( 100 );

	pinMode( GPIO_DEBUG, INPUT );
	debug_mode = (byte)(1 - gpio_get_level( GPIO_DEBUG )) | DEBUG_MODE;

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

			Serial.print( F( "Reboot counter=" ));
			Serial.println( reboot_count );

			if ( ntp_synced )
        		Serial.print( F( "NTP Synchronised. " ));
			else
				Serial.print( F( "NOT NTP Synchronised. " ));

			Serial.print( F( "Time and date: " ));
			Serial.println( &timeinfo, "%Y-%m-%d %H:%M:%S" );
		}

		initialise_sensors( &bme, &mlx, &tsl, &anemometer, &wind_vane, &rg9, &available_sensors, reboot_count, debug_mode );
		retrieve_sensor_data( values, &bme, &mlx, &tsl, &anemometer, &wind_vane, &rg9, ntp_synced, rain_event, &rain_intensity, debug_mode, &available_sensors );

		if ( rain_event ) {

			if ( rain_intensity > 0 )	// Avoid false positives
				send_rain_event_alarm( rain_intensity, debug_mode );

			else {

				if ( debug_mode )
					Serial.println( F( "Rain event false positive, back to bed." ));

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
		Serial.println( F( "Entering sleep mode." ));

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
 * --------------------------------
 *  
 *  Data handling
 * 
 * --------------------------------
*/
void retrieve_sensor_data( JsonDocument& values, Adafruit_BME280 *bme, Adafruit_MLX90614 *mlx, Adafruit_TSL2591 *tsl, SoftwareSerial *anemometer, SoftwareSerial *wind_vane, HardwareSerial *rg9, byte is_ntp_synced, byte rain_event, byte *rain_intensity, byte debug_mode, byte *available_sensors )
{
	float	temp,
			pres,
			rh,
			ambient_temp = 0,
			sky_temp = 0;
	time_t	now;

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
	values[ "ambient" ] = ambient_temp;
	values[ "sky" ] = sky_temp;

	values[ "direction" ] = read_wind_vane( wind_vane, available_sensors, debug_mode );
	values[ "speed" ] = read_anemometer( anemometer, available_sensors, debug_mode );

	*rain_intensity = read_RG9( rg9, debug_mode );
	values[ "rain" ] = *rain_intensity;

	values[ "sensors" ] = *available_sensors;
}

/*
 * --------------------------------
 *  
 *  Sensor intialisation
 * 
 * --------------------------------
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
	String		str;

	if ( reboot_count == 1 )    // Give time to the sensor to initialise before trying to probe it
		delay( 5000 );

	if ( debug_mode )
		Serial.printf( "Probing RG9 ... " );

	for ( int i = 0; i < RG9_PROBE_RETRIES; i++ )
		for ( int j = 0; j < RG9_SERIAL_SPEEDS; j++ ) {

			rg9->begin( bps[j], SERIAL_8N1, GPIO_RG9_RX, GPIO_RG9_TX );
			rg9->println();
			rg9->println();

			if ( debug_mode ) {
				Serial.printf( " trying %dbps: reading [", bps[j] );
				Serial.flush();
			}

			rg9->println( "B" );
			str = RG9_read_string( rg9 );
			str.trim();

			if ( debug_mode )
				Serial.print( str + "] " );

			if ( str.startsWith( "Baud " )) {

				if ( debug_mode ) {
					String bitrate = str.substring( 5, str.length() );
					Serial.println( "\nFound RG9 @ " + bitrate + "bps" );
				}

				*available_sensors |= RG9_SENSOR;
				return RG9_OK;
			}

			if ( str.startsWith( "Reset " )) {

				char reset_cause = str.charAt( 6 );
				if ( debug_mode )
					Serial.printf( "\nFound G9 @ %dbps after it was reset because of '%s'\n", bps[j], RG9_reset_cause( reset_cause ));

				while ( !(str = RG9_read_string( rg9 )).compareTo("")  ) Serial.print( str );

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
 * --------------------------------
 *  
 *  Utility functions
 * 
 * --------------------------------
*/
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
	
	Serial.println( F("\n##############################################################") );
	Serial.println( F("# AstroWeatherStation                                        #") );
	Serial.println( F("#  (c) Lesage Franck - lesage@datamancers.net                #") );
	snprintf( string, 64, "# Build %-52s #\n", REV );
	Serial.printf( string );
	Serial.println( F("#------------------------------------------------------------#") );
	snprintf( string, 64, "# %-58s #\n", wakeup_string );
	Serial.printf( string );
	Serial.println( F("#------------------------------------------------------------#") );
	Serial.println( F("# GPIO PIN CONFIGURATION                                     #") );
	Serial.println( F("#------------------------------------------------------------#") );
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
	Serial.println( F("##############################################################") );
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
		Serial.print( F( "Connecting to server..." ));

	client.setCACert( root_ca );

	if (!client.connect( server, 443 )) {

		if ( debug_mode )
			Serial.println( F( "NOK." ));

	} else {

		if ( debug_mode )
			Serial.println( F( "OK." ));

		final_endpoint = (char *)malloc( strlen( url ) + strlen( endpoint ) + 2 );
		strcpy( final_endpoint, url );
		strcat( final_endpoint, endpoint );
		http.begin( client, final_endpoint );
		http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
		http.addHeader( "Content-Type", "application/json" );
		status = http.POST( jsonString );

		if ( debug_mode ) {

			Serial.println();
			Serial.print( F( "HTTP response: " ));
			Serial.println( status );

		}

		client.stop();
		free( final_endpoint );
	}
}

void report_unavailable_sensors( byte available_sensors, byte debug_mode )
{
	const char sensor_name[6][12] = {"MLX96014 ", "TSL2591 ", "BME280 ", "WIND VANE ", "ANEMOMETER ", "RG9 "};
	char unavailable_sensors[96] = "Unavailable sensors: ";

	for ( byte i = 0; i < 6; i++ )
		if ( ( available_sensors & (int)pow( 2, i )) != (int)pow( 2, i ) )
			strncat( unavailable_sensors, sensor_name[i], 12 );

		if ( debug_mode ) {

			Serial.print( unavailable_sensors );
			if ( available_sensors == ALL_SENSORS )
				Serial.println( F( "none" ));
		}

	if ( available_sensors != ALL_SENSORS )
		send_alarm( "Unavailable sensors report", unavailable_sensors, debug_mode );
}

String RG9_read_string( HardwareSerial *rg9 )
{
	String str = "";

	delay( 500 );
	if ( rg9->available() > 0 )
		str = rg9->readString();

	return str;
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
		Serial.print( F( "Sending JSON: " ));
		serializeJson( values, Serial );
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

void send_rain_event_alarm( byte rain_instensity, byte debug_mode )
{
	const char  intensity_str[7][13] = { "Rain drops", "Very light", "Medium light", "Medium", "Medium heavy", "Heavy", "Violent" };  // As described in RG-9 protocol
	char 		msg[32];

	snprintf( msg, 32, "RAIN! Level = %s", intensity_str[ rain_instensity ] );
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
 * --------------------------------
 *  
 *  Sensor reading
 * 
 * --------------------------------
*/
float get_battery_level( byte debug_mode )
{
	if ( debug_mode )
		Serial.print( F( "Battery level: " ));

	digitalWrite( GPIO_BAT_ADC_EN, HIGH );
	delay( 1500 );
	int adc_value = analogRead( GPIO_BAT_ADC );
	float battery_level = map( adc_value, ADC_V_MIN, ADC_V_MAX, 0, 100 );

	if ( debug_mode ) {

		float adc_v_in = adc_value * VCC / ADC_V_MAX;
		float bat_v = adc_v_in * ( V_DIV_R1 + V_DIV_R2 ) / V_DIV_R2;
		Serial.printf( "%03.2f%% (ADC value=%d, ADC voltage=%1.3fV, battery voltage=%1.3fV)\n", battery_level, adc_value, adc_v_in / 1000, bat_v / 1000 );

	}

	digitalWrite( GPIO_BAT_ADC_EN, LOW );
	return battery_level;
}

float read_anemometer( SoftwareSerial *anemometer, byte *available_sensors, byte debug_mode )
{
	const byte  anemometer_request[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0a };
	byte		answer[7],
				i = 0,
				j;
	float wind_speed = -1;

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

			wind_speed = answer[4] / 10.0;

			if ( debug_mode )
				Serial.printf( "\nWind speed: %02.2f m/s\n", wind_speed );

		} else {

			if ( debug_mode )
				Serial.println( "(Error)." );
			delay( 500 );

		}

		if ( ++i == ANEMOMETER_MAX_TRIES ) {

			*available_sensors &= ~ANEMOMETER_SENSOR;
			wind_speed = -1;
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
		*pres = bme->readPressure() / 100.0;
		*rh = bme->readHumidity();

		if ( debug_mode ) {
			Serial.printf( "Temperature = %2.2f °C\n", *temp  );
			Serial.printf( "Pressure = %4.2f hPa\n", *pres );
			Serial.printf( "RH = %3.2f %%\n", *rh );
		}
		return;
	}

	*temp = -99.0;
	*pres = 0.0;
	*rh = -1.0;
}

void read_MLX( Adafruit_MLX90614 *mlx, float *amb, float *obj, byte available_sensors, byte debug_mode  )
{
	if ( ( available_sensors & MLX_SENSOR ) == MLX_SENSOR ) {

		*amb = mlx->readAmbientTempC();
		*obj = mlx->readObjectTempC();

		if ( debug_mode ) {
			Serial.print( "AMB = " );
			Serial.print( *amb );
			Serial.print( "°C SKY = " );
			Serial.print( *obj );
			Serial.println( "°C" );
		}
		return;
	}
	*amb = -99.0;
	*obj = -99.0;
}

byte read_RG9( HardwareSerial *rg9, byte debug_mode )
{
	rg9->println( "R" );
	String str = RG9_read_string( rg9 );

	if ( debug_mode )
		Serial.println( "RG9 STATUS = " + str );

	return (byte)str.substring( 2, 3 ).toInt();
}

int read_TSL( Adafruit_TSL2591 *tsl, byte available_sensors, byte debug_mode )
{
	int lux = -1;

	if ( ( available_sensors & TSL_SENSOR ) == TSL_SENSOR ) {

		uint32_t lum = tsl->getFullLuminosity();
		uint16_t ir, full;
		ir = lum >> 16;
		full = lum & 0xFFFF;
		lux = tsl->calculateLux( full, ir );

		if ( debug_mode )
			Serial.printf( "Lux = %06d\n", lux );

	}
	return lux;
}

int read_wind_vane( SoftwareSerial *wind_vane, byte *available_sensors, byte debug_mode )
{
	const byte  wind_vane_request[] = { 0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xc4, 0x38 };
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

			Serial.print( F( "Wind vane answer : " ));
			for ( j = 0; j < 6; j++ )
				Serial.printf( "%02x ", answer[j] );

		}

		if ( answer[1] == 0x03 ) {

			wind_direction = ( answer[5] << 8 ) + answer[6];

			if ( debug_mode )
				Serial.printf( "\nWind direction: %d°\n", wind_direction );

		} else {

			if ( debug_mode )
				Serial.println( F( "(Error)." ));
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
