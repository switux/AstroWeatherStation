/*
   AstroWeatherStation.ino

    (c) 2023 F.Lesage

    1.0 - Initial version.

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
#include <ESP_Mail_Client.h>

#define REV "1.0.3-20230314"
#define DEBUG_MODE 1

// You must customise weather_config.h
#include "weather_config.h"

#define GPIO_WIND_VANE_RX   27    // RO Pin
#define GPIO_WIND_VANE_TX   26    // DI Pin
#define GPIO_WIND_VANE_CTRL 14    // RE+DE Pins

#define GPIO_ANEMOMETER_RX   32    // RO Pin
#define GPIO_ANEMOMETER_TX   33    // DI Pin
#define GPIO_ANEMOMETER_CTRL 25    // RE+DE Pins

#define SEND  HIGH
#define RECV  LOW

#define WIND_VANE_MAX_TRIES   3   // Tests show that if there is no valid answer after 3 attempts, the sensor is not available
#define ANEMOMETER_MAX_TRIES  3   // Tests show that if there is no valid answer after 3 attempts, the sensor is not available

#define GPIO_RG9_RX       17            // J2 SO PIN
#define GPIO_RG9_TX       16            // J2 SI PIN
#define GPIO_RG9_MCLR     23            // J2 MCLR PIN
#define GPIO_RG9_RAIN     GPIO_NUM_35   // J1 OUT PIN
#define RG9_SERIAL_SPEEDS 7
#define RG9_PROBE_RETRIES 5
#define RG9_OK            0
#define RG9_FAIL          127
#define GPIO_RELAY_3_3V  05
#define GPIO_RELAY_12V    18

#define GPIO_DEBUG        GPIO_NUM_34

#define GPIO_BAT_LEVEL        13
#define GPIO_BAT_LVL_SW       12
#define BAT_LEVEL_MIN         50
#define LOW_BATTERY_COUNT_MIN 5
#define LOW_BATTERY_COUNT_MAX 10
#define ADC_VOLTAGE_MOD       (3.15 / 3.3)  // Max voltage given by divider if you cannot get a 366k resistor (I have a 300k)

#define US_SLEEP    5 * 60 * 1000000 // 5 minutes

#define RG9_UART  2

#define MLX_SENSOR        0x01
#define TSL_SENSOR        0x02
#define BME_SENSOR        0x04
#define WIND_VANE_SENSOR  0x08
#define ANEMOMETER_SENSOR 0x10
#define RG9_SENSOR        0x20
#define ALL_SENSORS       (MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RG9_SENSOR)

const char sensor_name[6][12] = {"MLX96014 ", "TSL2591 ", "BME280 ", "WIND-VANE ", "ANEMOMETER ", "RG9 "};

Adafruit_BME280 bme;
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_TSL2591 tsl = Adafruit_TSL2591( 2591 );
SoftwareSerial anemometer( GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX );
SoftwareSerial wind_vane( GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX );
HardwareSerial rg9( RG9_UART );

byte  sensors_found,
      debug_mode;

RTC_DATA_ATTR byte old_sensors;
RTC_DATA_ATTR byte low_battery_event_count;
RTC_DATA_ATTR byte reboot_count;

StaticJsonDocument<392> values;

const char  *ntp_server = "pool.ntp.org";
const char  *tz_info    = "CET-1CEST,M3.5.0,M10.5.0/3";

char wakeup_string[50];

ESP32Time rtc( 0 );
struct tm timeinfo;

void setup()
{
  float battery_level = 0.0;
  byte  ntp_retry = 5,
        ntp_synced = 0,
        i,
        rain,
        rain_event;
  char  string[64];

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  wakeup_reason_to_string( wakeup_reason );

  if ( wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 ) // It may not be a power issue but it is definitely not the timer or a rain event
    reboot_count++;
    
  Serial.begin( 115200 );
  delay( 100 );

  pinMode( GPIO_DEBUG, INPUT );
  debug_mode = (byte)(1 - gpio_get_level( GPIO_DEBUG )) | DEBUG_MODE;

  if ( debug_mode ) {
    Serial.printf( "\n##############################################################\n" );
    Serial.printf( "# AstroWeatherStation                                        #\n" );
    Serial.printf( "#  (c) Lesage Franck - lesage@datamancers.net                #\n" );
    snprintf( string, 64, "# Build %-52s #\n", REV );
    Serial.printf( string );
    Serial.printf( "#------------------------------------------------------------#\n" );
    snprintf( string, 64, "# %-58s #\n", wakeup_string );
    Serial.printf( string );
    Serial.printf( "#------------------------------------------------------------#\n" );
    Serial.printf( "# GPIO PIN CONFIGURATION                                     #\n" );
    Serial.printf( "#------------------------------------------------------------#\n" );
    memset( string, 0, 64 );
    snprintf( string, 61, "# Wind vane : RX=%d TX=%d CTRL=%d", GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX, GPIO_WIND_VANE_CTRL );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    memset( string, 0, 64 );
    snprintf( string, 61, "# Anemometer: RX=%d TX=%d CTRL=%d", GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX, GPIO_ANEMOMETER_CTRL );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    memset( string, 0, 64 );
    snprintf( string, 61, "# RG9       : RX=%d TX=%d MCLR=%d RAIN=%d", GPIO_RG9_RX, GPIO_RG9_TX, GPIO_RG9_MCLR, GPIO_RG9_RAIN );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    memset( string, 0, 64 );
    snprintf( string, 61, "# 3.3V RELAY: %d", GPIO_RELAY_3_3V );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    memset( string, 0, 64 );
    snprintf( string, 61, "# 12V RELAY : %d", GPIO_RELAY_12V );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    memset( string, 0, 64 );
    snprintf( string, 61, "# DEBUG     : %d", GPIO_DEBUG );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    memset( string, 0, 64 );
    snprintf( string, 61, "# BAT LVL   : SW=%d ADC=%d", GPIO_BAT_LVL_SW, GPIO_BAT_LEVEL );
    for( i = strlen( string ); i < 61; string[i++] = ' ' );
    strcat( string, "#\n" );
    Serial.printf( string );
    Serial.printf( "##############################################################\n" );
  }

  pinMode( GPIO_BAT_LVL_SW, OUTPUT );

  pinMode( GPIO_RELAY_3_3V, OUTPUT );
  pinMode( GPIO_RELAY_12V, OUTPUT );
  digitalWrite( GPIO_RELAY_3_3V, LOW );
  digitalWrite( GPIO_RELAY_12V, LOW );

  battery_level = get_battery_level();

  if ( connect_to_wifi() ) {

    rain_event = ( ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason );

    configTzTime( tz_info, ntp_server );

    while ( !( ntp_synced = getLocalTime( &timeinfo )) && ( --ntp_retry > 0 ) ) {
        delay( 1000 );
        configTzTime( tz_info, ntp_server );
    }

    if ( debug_mode ) {

      if ( ntp_synced )
        Serial.print( "NTP Synchronised. ");
      else
        Serial.print( "NOT NTP Synchronised. ");

      Serial.print( "Time and date: ");
      Serial.println( &timeinfo, "%Y-%m-%d %H:%M:%S" );
    }
    
    initialise_sensors();
    retrieve_sensor_data( ntp_synced, rain_event, &rain );
    
    if ( rain_event ) {

       if ( rain > 0 )  // Avoid false positives
        send_rain_event_alarm();

       else {

        if ( debug_mode )
          Serial.println( "Rain event false positive, back to bed." );

        goto enter_sleep;           // The hell with bigots and their aversion to goto's
       }

    }
    

    if ( battery_level <= BAT_LEVEL_MIN ) {

      memset( string, 0, 64 );
      snprintf( string, 61, "Battery level = %03.2f%%\n", battery_level );

      if (( low_battery_event_count++ >= LOW_BATTERY_COUNT_MIN ) && ( low_battery_event_count++ <= LOW_BATTERY_COUNT_MAX ))
        send_mail( "[Weather Station] Low battery", string );

      if ( debug_mode )
        Serial.printf( "%sLow battery event count = %d\n", string, low_battery_event_count );

    } else
      low_battery_event_count = 0;

      
    send_data();

    if ( old_sensors != sensors_found ) {

      old_sensors = sensors_found;
      report_missing_sensor();

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
  if ( debug_mode )
    Serial.println( "BUG! Entered the loop function." );
}

/*
 * Data handling
 */
 
void retrieve_sensor_data( byte has_ntp_time, byte rain_event, byte *rain )
{
  float temp, pres, rh;
  float ambient = 0, object = 0;

  values[ "timestamp" ] = ( has_ntp_time ? rtc.getEpoch() + ( rtc.getMicros() / 1000000 ) : 0 );
  values[ "rain_event" ] = rain_event;

  read_BME( &temp, &pres, &rh );
  values[ "temp" ] = temp;
  values[ "pres" ] = pres;
  values[ "rh" ] = rh;

  values[ "lux" ] = read_TSL();

  read_MLX( &ambient, &object);
  values[ "ambient" ] = ambient;
  values[ "sky" ] = object;

  values[ "direction" ] = read_wind_vane();
  values[ "speed" ] = read_anemometer();

  *rain = read_RG9();
  values[ "rain" ] = *rain;

  values[ "sensors" ] = sensors_found;
}

/*
 * Sensor intialisation
 */
 
void initialise_sensors()
{
  initialise_BME();
  initialise_MLX();
  initialise_TSL();
  initialise_anemometer();
  initialise_wind_vane();
  switch( initialise_RG9() ) {
    case RG9_OK:
    case RG9_FAIL:
      break;
    case 'B':
        send_mail( "[Weather Station] RG9 low voltage", "" );
        break;
    case 'O':
        send_mail( "[Weather Station] RG9 problem", "Reset because of stack overflow, report problem to support." );
        break;
    case 'U':
        send_mail( "[Weather Station] RG9 problem", "Reset because of stack underflow, report problem to support." );
        break;
    default:
        break;
  }
}

void initialise_anemometer()
{
  // TODO: Test ESP32's specific RS485 mode
  anemometer.begin( 9600 );
  pinMode( GPIO_ANEMOMETER_CTRL, OUTPUT );
}

void initialise_BME()
{
  if ( !bme.begin( 0x76 ) ) {

    if ( debug_mode )
      Serial.println( "Could not find BME280." );

  } else {
    
    if ( debug_mode )
      Serial.println( "Found BME280." );

    sensors_found |= BME_SENSOR;

  }
}

void initialise_MLX()
{
  if ( !mlx.begin() ) {

    if ( debug_mode )
      Serial.println( "Could not find MLX90614" );

  } else {

    if ( debug_mode )
      Serial.println( "Found MLX90614" );

    sensors_found |= MLX_SENSOR;

  }
}

char initialise_RG9()
{
  int     bps[ RG9_SERIAL_SPEEDS ] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600 };
  String  str;

  if ( reboot_count == 1 )    // Give time to the sensor to initialise before trying to probe it
    delay( 5000 );
    
  if ( debug_mode )
    Serial.printf( "Probing RG9 ... " );

  for( int i = 0; i < RG9_PROBE_RETRIES; i++ )
    for( int j = 0; j < RG9_SERIAL_SPEEDS; j++ ) {

      rg9.begin( bps[j], SERIAL_8N1, GPIO_RG9_RX, GPIO_RG9_TX );
      rg9.println();
      rg9.println();

      if ( debug_mode ) {
        Serial.printf( " trying %dbps: reading [", bps[j] );
        Serial.flush();
      }
      
      rg9.println( "B" );
      str = RG9_read_string();
      str.trim();
      
      if ( debug_mode )
        Serial.print( str+"] " );

      if ( str.startsWith( "Baud " )) {

        if ( debug_mode ) {
         String bitrate = str.substring( 5, str.length() );
         Serial.println( "\nFound RG9 @ " + bitrate + "bps" );
        }

        sensors_found |= RG9_SENSOR;
        return RG9_OK;
      }

      if ( str.startsWith( "Reset " )) {

        char reset_cause = str.charAt( 6 );
        if ( debug_mode )
         Serial.printf( "\nFound G9 @ %dbps after it was reset because of '%s'\n", bps[j], RG9_reset_cause( reset_cause ));

        while ( !(str = RG9_read_string()).compareTo("")  ) Serial.print( str );
        
        sensors_found |= RG9_SENSOR;
        return reset_cause;
      }

      rg9.end();
    }

  if ( debug_mode )
    Serial.println( "Could not find RG9, resetting..." );

  pinMode( GPIO_RG9_MCLR, OUTPUT );
  digitalWrite( GPIO_RG9_MCLR, LOW );
  delay( 500 );
  digitalWrite( GPIO_RG9_MCLR, HIGH );

  return RG9_FAIL;
}

void initialise_TSL()
{
  if ( !tsl.begin() ) {

    if ( debug_mode )
      Serial.println( "Could not find TSL2591" );

  } else {

    if ( debug_mode )
      Serial.println( "Found TSL2591" );

    tsl.setGain( TSL2591_GAIN_LOW );
    tsl.setTiming( TSL2591_INTEGRATIONTIME_100MS );
    sensors_found |= TSL_SENSOR;

  }
}

void initialise_wind_vane()
{
  // TODO: Test ESP32's specific RS485 mode
  wind_vane.begin( 9600 );
  pinMode( GPIO_WIND_VANE_CTRL, OUTPUT );
}


/*
 * Utility functions
 */
 
int connect_to_wifi()
{
  if ( debug_mode )
    Serial.printf( "Attempting to connect to SSID [%s] ", ssid );

  WiFi.begin( ssid, password );
  byte remaining_attempts = 10;

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

void report_missing_sensor()
{
  char lost_sensors[88] = "Lost sensors: ";

  for ( byte i = 0; i < 6; i++ )
    if ( ( sensors_found & (int)pow( 2, i )) != (int)pow( 2, i ) )
      strncat( lost_sensors, sensor_name[i], 12 );

  if ( debug_mode )
    Serial.println( lost_sensors );

  if ( strlen( lost_sensors ) > 14 )
    send_mail( "[Weather Station] Lost sensor", lost_sensors );
}

String RG9_read_string()
{
  String str = "";
  delay( 500 );

  if ( rg9.available() > 0 )
    str = rg9.readString();

  return str;
}

char *RG9_reset_cause( char code )
{
  switch( code ) {
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

void send_data()
{
  WiFiClientSecure client;
  char json[400];
  HTTPClient http;
  int status;

  if ( debug_mode )
    Serial.print( "Connecting to server..." );

  client.setCACert( root_ca );

  if (!client.connect( server, 443 )) {

    if ( debug_mode )
      Serial.println( "NOK." );

  } else {

    if ( debug_mode )
      Serial.println( "OK." );

    serializeJson( values, json );

    if ( debug_mode ) {

      Serial.print( "Sending JSON: " );
      serializeJson( values, Serial );

    }

    http.begin( client, url );
    http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
    http.addHeader( "Content-Type", "application/json" );
    status = http.POST( json );

    if ( debug_mode ) {

      Serial.println();
      Serial.print( "HTTP response: " );
      Serial.println( status );

    }

    client.stop();
  }
}

void send_mail( const char *subject, const char *body )
{
  SMTPSession       smtp;
  ESP_Mail_Session  session;
  SMTP_Message      message;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = FROM;
  session.login.password = SMTP_PASSWORD;
  session.login.user_domain = "";

  message.sender.name = "Weather Station";
  message.sender.email = FROM;
  message.subject = subject;
  message.text.content = body;

  message.addRecipient( "Me", TO );

  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  smtp.connect( &session );
  MailClient.sendMail(&smtp, &message);
}

void send_rain_event_alarm()
{
  send_mail( "[Weather Station] Rain event", "RAIN! RAIN!" );
}

void wakeup_reason_to_string( esp_sleep_wakeup_cause_t wakeup_reason )
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
 * Sensor reading
 */
 
float get_battery_level()
{
  if ( debug_mode )
    Serial.print( "Battery level: " );
    
  digitalWrite( GPIO_BAT_LVL_SW, HIGH );
  delay( 1500 );
  int adc_value = analogRead( GPIO_BAT_LEVEL );
  float battery_level = map( adc_value / ADC_VOLTAGE_MOD, 0.0f, 4095.0f, 0, 100 );

  if ( debug_mode )
    Serial.printf( "%03.2f%% (ADC value=%d)\n", battery_level, adc_value );
    
  digitalWrite( GPIO_BAT_LVL_SW, LOW );
  values[ "battery_level" ] = battery_level;
  return battery_level;
}

float read_anemometer()
{
  byte  anemometer_request[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0a },
                               answer[7];
  float wind_speed = -1;
  byte  i = 0,
        j;

  answer[1] = 0;

  while ( answer[1] != 0x03 ) {

    digitalWrite( GPIO_ANEMOMETER_CTRL, SEND );
    anemometer.write( anemometer_request, 8 );
    anemometer.flush();

    digitalWrite( GPIO_ANEMOMETER_CTRL, RECV );
    anemometer.readBytes( answer, 7 );

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

      sensors_found &= ~ANEMOMETER_SENSOR;
      wind_speed = -1;
      break;
    }
  }
  sensors_found |= ANEMOMETER_SENSOR;
  return wind_speed;
}

void read_BME( float *temp, float *pres, float *rh )
{
  if ( ( sensors_found & BME_SENSOR ) == BME_SENSOR ) {

    sensors_event_t temp_event,
                    pressure_event,
                    humidity_event;

    bme_temp->getEvent( &temp_event );
    bme_pressure->getEvent( &pressure_event );
    bme_humidity->getEvent( &humidity_event );

    if ( debug_mode ) {

      Serial.printf( "Temperature = %2.2f °C\n", temp_event.temperature  );
      Serial.printf( "Pressure = %4.2f hPa\n", pressure_event.pressure );
      Serial.printf( "RH = %3.2f %%\n", humidity_event.relative_humidity );

    }

    *temp = temp_event.temperature;
    *pres = pressure_event.pressure;
    *rh = humidity_event.relative_humidity;
    return;
  }

  *temp = -99.0;
  *pres = 0.0;
  *rh = -1.0;
}

void read_MLX( float *amb, float *obj )
{
  if ( ( sensors_found & MLX_SENSOR ) == MLX_SENSOR ) {

    *amb = mlx.readAmbientTempC();
    *obj = mlx.readObjectTempC();

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

byte read_RG9()
{
  rg9.println( "R" );
  String str = RG9_read_string();

  if ( debug_mode )
    Serial.println( "RG9 STATUS = " + str );
  
  return (byte)str.substring( 2, 3 ).toInt();
}

int read_TSL()
{
  int lux = -1;

  if ( ( sensors_found & TSL_SENSOR ) == TSL_SENSOR ) {

    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ir, full;
    ir = lum >> 16;
    full = lum & 0xFFFF;
    lux = tsl.calculateLux( full, ir );

    if ( debug_mode )
      Serial.printf( "Lux = %06d\n", lux );

  }
  return lux;
}

int read_wind_vane()
{
  byte  wind_vane_request[] = { 0x02, 0x03, 0x00, 0x00, 0x00, 0x02, 0xc4, 0x38 },
                              answer[7],
                              i = 0,
                              j;
  int   wind_direction = -1;

  answer[1] = 0;

  while ( answer[1] != 0x03 ) {

    digitalWrite( GPIO_WIND_VANE_CTRL, SEND );
    wind_vane.write( wind_vane_request, 8 );
    wind_vane.flush();

    digitalWrite( GPIO_WIND_VANE_CTRL, RECV );
    wind_vane.readBytes( answer, 7 );

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

      sensors_found &= ~WIND_VANE_SENSOR;
      wind_direction = -1;
      break;
    }
  }
  sensors_found |= WIND_VANE_SENSOR;
  return wind_direction;
}
