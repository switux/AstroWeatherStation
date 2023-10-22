#include <Arduino.h>
#include <time.h>
#include <thread>
#include <Ethernet.h>
#include <SSLClient.h>
#include <AsyncUDP_ESP32_W5500.h>
#include <ESPAsyncWebSrv.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESP32OTAPull.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <ESPping.h>
#include <stdio.h>
#include <stdarg.h>

#include "defaults.h"
#include "gpio_config.h"
#include "SC16IS750.h"
#include "GPS.h"
#include "AstroWeatherStation.h"
#include "SQM.h"
#include "SC16IS750.h"
#include "AWSSensorManager.h"
#include "AWSConfig.h"
#include "AWSWeb.h"
#include "alpaca.h"
#include "AWS.h"

extern void IRAM_ATTR _handle_rain_event( void );
extern char *pwr_mode_str[3];

AstroWeatherStation::AstroWeatherStation( void )
{
	rain_event = false;
	config_mode = false;
	debug_mode = false;
	config = new AWSConfig();
	json_sensor_data = (char *)malloc( 1024 );
}

void AstroWeatherStation::check_ota_updates( void )
{
	ESP32OTAPull	ota;
	char			string[64];
	int				ret_code;

	snprintf( string, 64, "%s_%d", ESP.getChipModel(), ESP.getChipRevision() );
	ota.OverrideBoard( string );
	snprintf( string, 64, "%02x:%02x:%02x:%02x:%02x:%02x", wifi_mac[0], wifi_mac[1], wifi_mac[2], wifi_mac[3], wifi_mac[4], wifi_mac[5] );
	ota.OverrideDevice( string );
	snprintf( string, 64, "%d-%s-%s-%s", config->get_pwr_mode(), config->get_pcb_version(), REV, BUILD_DATE );
	ota.SetConfig( string );
	ota.SetCallback( OTA_callback );
	Serial.printf( "[INFO] Checking for OTA firmware update...\n" );
	ret_code = ota.CheckForOTAUpdate( "https://www.datamancers.net/images/AWS.json", REV );
	Serial.printf( "[INFO] Firmware OTA update result: %s\n", OTA_message( ret_code ));
}

IPAddress AstroWeatherStation::cidr_to_mask( byte cidr )
{
	uint32_t		subnet;
	
	if ( cidr )
		subnet = htonl( ~(( 1 << ( 32 - cidr )) -1 ) );
	else
		subnet = htonl( 0 );

	return IPAddress( subnet );
}

bool AstroWeatherStation::connect_to_wifi()
{
	uint8_t		remaining_attempts	= 10;

	const char	*ssid		= config->get_sta_ssid(),
				*password	= config->get_wifi_sta_password();

	if (( WiFi.status () == WL_CONNECTED ) && !strcmp( ssid, WiFi.SSID().c_str() )) {

		if ( debug_mode )
			Serial.printf( "[DEBUG] Already connected to SSID [%s].\n", ssid );
		return true;
	}

	Serial.printf( "[INFO] Attempting to connect to SSID [%s] ", ssid );

	WiFi.begin( ssid , password );

	while (( WiFi.status() != WL_CONNECTED ) && ( --remaining_attempts > 0 )) {

		Serial.print( "." );
		delay( 1000 );
	}

	if ( WiFi.status () == WL_CONNECTED ) {

		Serial.printf( " OK. Using IP [%s]\n", WiFi.localIP().toString().c_str() );
		return true;
	}

	Serial.println( "NOK." );

	return false;
}

bool AstroWeatherStation::disconnect_from_wifi( void )
{
	byte i = 0;

	WiFi.disconnect();
	while ( (i++ < 20) && ( WiFi.status() == WL_CONNECTED ))
    	delay( 100 );

	return ( WiFi.status() != WL_CONNECTED );
}

void AstroWeatherStation::display_banner()
{
	Serial.println( "\n##############################################################################################" );
	Serial.println( "# AstroWeatherStation                                                                        #" );
	Serial.println( "#  (c) Lesage Franck - lesage@datamancers.net                                                #" );
	Serial.println( "#--------------------------------------------------------------------------------------------#" );

	print_config_string( "# MCU              : Model %s Revision %d", ESP.getChipModel(), ESP.getChipRevision() );
	print_config_string( "# WIFI Mac         : %02x:%02x:%02x:%02x:%02x:%02x", wifi_mac[0], wifi_mac[1], wifi_mac[2], wifi_mac[3], wifi_mac[4], wifi_mac[5] );
	print_config_string( "# Power Mode       : %s", pwr_mode_str[ config->get_pwr_mode() ] );
	print_config_string( "# PCB version      : %s", config->get_pcb_version() );
	print_config_string( "# Anemometer model : %s", config->get_anemometer_model_str() );
	print_config_string( "# Windvane model   : %s", config->get_wind_vane_model_str() );
	print_config_string( "# Ethernet present : %s", config->get_has_ethernet() ? "Yes" : "No" );
	print_config_string( "# Firmware         : %s-%s", REV, BUILD_DATE );

	Serial.println( "#--------------------------------------------------------------------------------------------#" );
	Serial.println( "# GPIO PIN CONFIGURATION                                                                     #" );
	Serial.println( "#--------------------------------------------------------------------------------------------#" );

	print_config_string( "# Wind vane  : RX=%d TX=%d CTRL=%d", GPIO_WIND_VANE_RX, GPIO_WIND_VANE_TX, GPIO_WIND_VANE_CTRL );
	print_config_string( "# Anemometer : RX=%d TX=%d CTRL=%d", GPIO_ANEMOMETER_RX, GPIO_ANEMOMETER_TX, GPIO_ANEMOMETER_CTRL );
	print_config_string( "# RG9        : RX=%d TX=%d MCLR=%d RAIN=%d", GPIO_RG9_RX, GPIO_RG9_TX, GPIO_RG9_MCLR, GPIO_RG9_RAIN );
	print_config_string( "# 3.3V SWITCH: %d", GPIO_ENABLE_3_3V );
	print_config_string( "# 12V SWITCH : %d", GPIO_ENABLE_12V );
	print_config_string( "# DEBUG      : %d", GPIO_DEBUG );
	print_config_string( "# BAT LVL    : SW=%d ADC=%d", GPIO_BAT_ADC_EN, GPIO_BAT_ADC );

	Serial.println( "#--------------------------------------------------------------------------------------------#" );
	Serial.println( "# RUNTIME CONFIGURATION                                                                      #" );
	Serial.println( "#--------------------------------------------------------------------------------------------#" );

	print_runtime_config();

	Serial.println( "##############################################################################################" );
}

void AstroWeatherStation::enter_config_mode( void )
{
	if ( debug_mode )
		Serial.printf( "[INFO] Entering config mode...\n ");

	start_config_server();
	Serial.printf( "[ERROR] Failed to start WiFi AP, cannot enter config mode.\n ");
}

byte AstroWeatherStation::get_ap_cidr_prefix( void )
{
	return mask_to_cidr( (uint32_t)ap_subnet );
}

IPAddress *AstroWeatherStation::get_ap_dns( void )
{
	return &ap_dns;
}

IPAddress *AstroWeatherStation::get_ap_gw( void )
{
	return &ap_gw;
}

IPAddress *AstroWeatherStation::get_ap_ip( void )
{
	return &ap_ip;
}

uint16_t AstroWeatherStation::get_config_port( void )
{
	return config->get_config_port();
}

byte AstroWeatherStation::get_eth_cidr_prefix( void )
{
	return mask_to_cidr( (uint32_t)eth_subnet );
}

IPAddress *AstroWeatherStation::get_eth_dns( void )
{
	return &eth_dns;
}

IPAddress *AstroWeatherStation::get_eth_gw( void )
{
	return &eth_gw;
}

IPAddress *AstroWeatherStation::get_eth_ip( void )
{
	return &eth_ip;
}

char *AstroWeatherStation::get_json_sensor_data( void )
{
	DynamicJsonDocument	json_data(768);
	sensor_data_t		*sensor_data = sensor_manager.get_sensor_data();

	json_data["battery_level"] = sensor_data->battery_level;
	json_data["timestamp"] = sensor_data->timestamp;
	json_data["rain_event"] = sensor_data->rain_event;
	json_data["temperature"] = sensor_data->temperature;
	json_data["pressure"] = sensor_data->pressure;
	json_data["sl_pressure"] = sensor_data->sl_pressure;
	json_data["rh"] = sensor_data->rh;
	json_data["ota_board"] = sensor_data->ota_board;
	json_data["ota_device"] = sensor_data->ota_device;
	json_data["ota_config"] = sensor_data->ota_config;
	json_data["wind_speed"] = sensor_data->wind_speed;
	json_data["wind_direction"] = sensor_data->wind_direction;
	json_data["dew_point"] = sensor_data->dew_point;
	json_data["rain_intensity"] = sensor_data->rain_intensity;
	json_data["sky_temperature"] = sensor_data->sky_temperature;
	json_data["ambient_temperature"] = sensor_data->ambient_temperature;
	json_data["msas"] = sensor_data->msas;
	json_data["nelm"] = sensor_data->nelm;
	json_data["integration_time"] = sensor_data->integration_time;
	json_data["gain"] = sensor_data->gain;
	json_data["ir_luminosity"] = sensor_data->ir_luminosity;
	json_data["full_luminosity"] = sensor_data->full_luminosity;
	json_data["lux"] = sensor_data->lux;
	json_data["irradiance"] = sensor_data->irradiance;
	json_data["ntp_time_sec"] = sensor_data->ntp_time.tv_sec;
	json_data["ntp_time_usec"] = sensor_data->ntp_time.tv_usec;
	json_data["gps_fix"] = sensor_data->gps.fix;
	json_data["gps_longitude"] = sensor_data->gps.longitude;
	json_data["gps_latitude"] = sensor_data->gps.latitude;
	json_data["gps_altitude"] = sensor_data->gps.altitude;
	json_data["gps_time_sec"] = sensor_data->gps.time.tv_sec;
	json_data["gps_time_usec"] = sensor_data->gps.time.tv_usec;
	json_data["uptime"] = get_uptime();

	serializeJson( json_data, json_sensor_data, 1024 );
	return json_sensor_data;
}

char *AstroWeatherStation::get_json_string_config( void )
{
	return config->get_json_string_config();
}

char* AstroWeatherStation::get_root_ca( void )
{
	return config->get_root_ca();
}

sensor_data_t *AstroWeatherStation::get_sensor_data( void )
{
	return sensor_manager.get_sensor_data();
}

char *AstroWeatherStation::get_uptime( void )
{
	int64_t uptime_s = round( esp_timer_get_time() / 1000000 );
	int days = floor( uptime_s / ( 3600 * 24 ));
	int hours = floor( fmod( uptime_s, 3600 * 24 ) / 3600 );
	int minutes = floor( fmod( uptime_s, 3600 ) / 60 );
	int seconds = fmod( uptime_s, 60 );

	sprintf( uptime, "%llu %03dd:%02dh:%02dm:%02ds", uptime_s, days, hours, minutes, seconds );
	return uptime;
}

byte AstroWeatherStation::get_sta_cidr_prefix( void )
{
	return mask_to_cidr( (uint32_t)sta_subnet );
}

IPAddress *AstroWeatherStation::get_sta_dns( void )
{
	return &sta_dns;
}

IPAddress *AstroWeatherStation::get_sta_gw( void )
{
	return &sta_gw;
}

IPAddress *AstroWeatherStation::get_sta_ip( void )
{
	return &sta_ip;
}

void AstroWeatherStation::handle_rain_event( void )
{
	Serial.printf("RAIN\n");
	// FIXME: double?
	  rain_event = true;
	sensor_manager.set_rain_event();
	detachInterrupt( GPIO_RG9_RAIN );
}

bool AstroWeatherStation::initialise( void )
{
	unsigned long	start = micros(),
					guard;
	int				rain_intensity;

 	pinMode( GPIO_DEBUG, INPUT );

	debug_mode = static_cast<bool>( 1 - gpio_get_level( GPIO_DEBUG )) | DEBUG_MODE;
	while ( (( micros() - start ) <= CONFIG_MODE_GUARD ) && !( gpio_get_level( GPIO_DEBUG ))) {
		delay( 100 );
		guard = micros() - start;
	}

	config_mode = ( guard > CONFIG_MODE_GUARD );

	Serial.printf( "[INFO] AstroWeatherStation is booting...\n" );

	if ( !config->load( debug_mode ) )
		return false;

	if ( !initialise_network()) {

		config->rollback();
		return false;
	}

	solar_panel = ( config->get_pwr_mode() == panel );

	if ( solar_panel ) {

		esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
		rain_event = ( ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason );

		if ( config_mode )
			enter_config_mode();

	} else {

		start_config_server();
	}

	alpaca = new alpaca_server();
	switch ( config->get_alpaca_iface() ) {

		case sta:
			alpaca->start( WiFi.localIP(), debug_mode  );
			break;

		case ap:
			alpaca->start( WiFi.softAPIP(), debug_mode  );
			break;

		case eth:
			alpaca->start( Ethernet.localIP(), debug_mode  );
			break;

	}

	if ( !startup_sanity_check() ) {

		config->rollback();
		return false;
	}
  
	if ( debug_mode )
		display_banner();

	if ( rain_event ) {

		Serial.printf("RAIN EVENT\n");
		sensor_manager.initialise( config, debug_mode );
		sensor_manager.initialise_RG9();
		sensor_manager.read_RG9();
		rain_intensity = sensor_manager.get_sensor_data()->rain_intensity;
		if ( rain_intensity > 0 )	// Avoid false positives
			send_rain_event_alarm( rain_intensity );
		else
			Serial.println( "[INFO] Rain event false positive, back to bed." );
		return true;
	}

	if ( !sensor_manager.initialise( config, debug_mode ))
		return false;

	// QUESTION: move earlier?
	if ( !solar_panel ) {

		if ( config->get_has_rg9() ) {

			pinMode( GPIO_RG9_RAIN, INPUT );
			attachInterrupt( GPIO_RG9_RAIN, _handle_rain_event, FALLING );
		}
	}

	return true;
}

bool AstroWeatherStation::initialise_ethernet( void )
{
	char	*ip = NULL,
			*cidr = NULL,
			buf[32];

	pinMode( 4, OUTPUT );
	digitalWrite( 4, HIGH );
	delay( 250 );
	digitalWrite( 4, LOW );
	delay( 50 );
	digitalWrite( 4, HIGH );
	delay( 350 );

	ESP32_W5500_onEvent();

	// You will have to change esp_w5500.cpp ETH.begin() to make it accept eth_mac even if a mac already exists,
	// honestly I do not understand why you would silently override the eth_mac parameter
	if ( !ETH.begin( GPIO_SPI_MISO, GPIO_SPI_MOSI, GPIO_SPI_SCK, GPIO_SPI_CS_ETH, GPIO_SPI_INT, SPI_CLOCK_MHZ, SPI3_HOST, eth_mac )) {

		Serial.printf( "[ERROR] Could not initialise ethernet!\n" );
		return false;
	}

  	if ( config->get_eth_ip_mode() == fixed ) {

		if ( config->get_eth_ip() ) {

			strcpy( buf, config->get_eth_ip() );
		 	ip = strtok( buf, "/" );
		}
		if ( ip )
			cidr = strtok( NULL, "/" );

  		eth_ip.fromString( ip );
  		eth_gw.fromString( config->get_eth_gw() );
  		eth_dns.fromString( config->get_eth_dns() );
		ETH.config( eth_ip, eth_gw, cidr_to_mask( atoi( cidr )), eth_dns );
  	}

	ESP32_W5500_waitForConnect();
	
	eth_ip = ETH.localIP();
	eth_gw = ETH.gatewayIP();
	eth_subnet = ETH.subnetMask();
	eth_dns = ETH.dnsIP();
	
	return true;
}

bool AstroWeatherStation::initialise_network( void )
{
	esp_read_mac( wifi_mac, ESP_MAC_WIFI_STA );

	switch ( config->get_pref_iface() ) {

		case wifi_ap:
		case wifi_sta:
			return initialise_wifi();
		case eth:
			return initialise_ethernet();
		default:
			Serial.printf( "[ERROR] Invalid prefered iface, falling back to WiFi.\n" );
			initialise_wifi();
			return false;
	}
	return false;
}

void AstroWeatherStation::initialise_sensors( void )
{
	sensor_manager.initialise_sensors();
}

bool AstroWeatherStation::initialise_wifi( void )
{
	switch ( config->get_wifi_mode() ) {

		case ap:
			if ( debug_mode )
				Serial.printf( "[DEBUG] Booting in AP mode.\n" );
			WiFi.mode( WIFI_AP );
			return start_hotspot();
		case both:
			if ( debug_mode )
				Serial.printf( "[DEBUG] Booting in AP+STA mode.\n" );
			WiFi.mode( WIFI_AP_STA );
			return ( start_hotspot() & connect_to_wifi() );
		case sta:
			if ( debug_mode )
				Serial.printf( "[DEBUG] Booting in Ethernet mode.\n" );
			WiFi.mode( WIFI_STA );
			return connect_to_wifi();
		default:
			Serial.printf( "[ERROR] Unknown wifi mode [%d]\n", config->get_wifi_mode() );
	}
	return false;
}

bool AstroWeatherStation::is_rain_event( void )
{
	return rain_event;
}

byte AstroWeatherStation::mask_to_cidr( uint32_t subnet )
{
	byte		cidr 	= 0;
	uint32_t	mask 	= 0x80000000,
				x 		= ntohl( subnet );

	while ( mask != 0 ) {
		if (( x & mask ) != mask )
			break;
		cidr++;
		mask >>= 1;
	}
	return cidr;
}

bool AstroWeatherStation::on_solar_panel( void )
{
	return solar_panel;
}

void OTA_callback( int offset, int total_length )
{
	static float	percentage = 0.F;
	float			p = static_cast<float>( 100.F * offset / total_length );

	if ( p - percentage > 10.F ) {

		Serial.printf("[INFO] Updating %d of %d (%02d%%)...\n", offset, total_length, 100 * offset / total_length );
		percentage = p;
	}
}

const char *AstroWeatherStation::OTA_message( int code )
{
	switch ( code ) {
		case ESP32OTAPull::UPDATE_AVAILABLE:
			return "An update is available but wasn't installed";
		case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
			return "No profile matches";
		case ESP32OTAPull::NO_UPDATE_AVAILABLE:
			return "Profile matched, but update not applicable";
		case ESP32OTAPull::UPDATE_OK:
			return "An update was done, but no reboot";
		case ESP32OTAPull::HTTP_FAILED:
			return "HTTP GET failure";
		case ESP32OTAPull::WRITE_ERROR:
			return "Write error";
		case ESP32OTAPull::JSON_PROBLEM:
			return "Invalid JSON";
		case ESP32OTAPull::OTA_UPDATE_FAIL:
			return "Update fail (no OTA partition?)";
		default:
			if ( code > 0 )
				return "Unexpected HTTP response code";
			break;
	}
	return "Unknown error";
}

void AstroWeatherStation::post_content( const char *endpoint, const char *jsonString )
{
	const char	*server = config->get_remote_server(),
				*url_path = config->get_url_path();
	char		*url;

  WiFiClientSecure wifi_client;
  HTTPClient http;

  uint8_t status = 0;
  char *final_endpoint;

  url = (char *)malloc( 7 + strlen( server ) + 1 + strlen( url_path ) + 3 );
  sprintf( url, "https://%s/%s/", server, url_path );

  if ( debug_mode )
    Serial.printf( "[DEBUG] Connecting to server [%s:443] ...", server );

  final_endpoint = (char *)malloc( strlen( url ) + strlen( endpoint ) + 2 );
  strcpy( final_endpoint, url );
  strcat( final_endpoint, endpoint );

  if ( config->get_pref_iface() == eth ) {

    http.begin( final_endpoint );
    http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
    http.addHeader( "Host", server );
    http.addHeader( "Accept", "application/json" );
    http.addHeader( "Content-Type", "application/json" );
    http.POST( jsonString );
    http.end();

  } else {

    wifi_client.setCACert( config->get_root_ca() );
    if (!wifi_client.connect( server, 443 )) {

      if ( debug_mode )
        Serial.println( "NOK." );

    } else {

      if ( debug_mode )
        Serial.println( "OK." );
    }

    http.begin( wifi_client, final_endpoint );
    http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
    http.addHeader( "Content-Type", "application/json" );
    status = http.POST( jsonString );
    if ( debug_mode ) {

      Serial.println();
      Serial.print( "[DEBUG] HTTP response: " );
      Serial.println( status );
    }
    http.end();
    //wifi_client.stop();
    free( final_endpoint );
  }
}

void AstroWeatherStation::print_config_string( const char *fmt, ... )
{
  	char	string[96];
	byte 	i;
	va_list	args;

	memset( string, 0, 96 );
	va_start( args, fmt );
	vsnprintf( string, 93, fmt, args );
	va_end( args );
	for( i = strlen( string ); i < 93; string[ i++ ] = ' ' );
	strcat( string, "#\n" );
	Serial.printf( string );
}

void AstroWeatherStation::print_runtime_config( void )
{
  char		_string[96];
  int			offset,
          len,
          room,
          s;
  uint8_t		pad,
            j;

  print_config_string( "# AP SSID      : %s", config->get_ap_ssid() );
  print_config_string( "# AP PASSWORD  : %s", config->get_wifi_ap_password() );
  print_config_string( "# AP IP        : %s", config->get_wifi_ap_ip() );
  print_config_string( "# AP Gateway   : %s", config->get_wifi_ap_gw() );
  print_config_string( "# STA SSID     : %s", config->get_sta_ssid() );
  print_config_string( "# STA PASSWORD : %s", config->get_wifi_sta_password() );
  print_config_string( "# STA IP       : %s", config->get_wifi_sta_ip() );
  print_config_string( "# STA Gateway  : %s", config->get_wifi_sta_gw() );
  print_config_string( "# SERVER       : %s", config->get_remote_server() );
  print_config_string( "# URL PATH     : /%s", config->get_url_path() );
  print_config_string( "# TZNAME       : %s", config->get_tzname() );

  memset( _string, 0, 96 );
  snprintf( _string, 93, "# ROOT CA   : " );
  len = strlen( config->get_root_ca() );
  offset = 0;
  while ( offset < len ) {
    s = strlen( _string );
    room = 96 - s - 4;
    strncat( _string, config->get_root_ca() + offset, room );
    offset += strlen( _string ) - s;
    for ( pad = strlen( _string ); pad < 93; _string[pad++] = ' ' );
    for ( j = 0; j < strlen(_string); j++ )
      if ( _string[j] == '\n' ) _string[j] = ' ';
    strcat( _string, "#\n" );
    Serial.printf( _string );
    memset( _string, 0, 96 );
    sprintf( _string, "# " );
  }

  print_config_string( "# SQM/SOL.IRR. : %s", config->get_has_tsl() ? "Yes" : "No" );
  print_config_string( "# CLOUD SENSOR : %s", config->get_has_mlx() ? "Yes" : "No" );
  print_config_string( "# RH/TEMP/PRES : %s", config->get_has_bme() ? "Yes" : "No" );
  print_config_string( "# WINDVANE     : %s", config->get_has_wv() ? "Yes" : "No" );
  print_config_string( "# ANEMOMETER   : %s", config->get_has_ws() ? "Yes" : "No" );
  print_config_string( "# RAIN SENSOR  : %s", config->get_has_rg9() ? "Yes" : "No" );
}

void AstroWeatherStation::read_sensors( void )
{
  sensor_manager.read_sensors();
}

void AstroWeatherStation::reboot( void )
{
  ESP.restart();
}

void AstroWeatherStation::report_unavailable_sensors( void )
{
  const char	sensor_name[6][12] = {"MLX96014 ", "TSL2591 ", "BME280 ", "WIND VANE ", "ANEMOMETER ", "RG9 "};
  char		unavailable_sensors[96] = "Unavailable sensors: ";
  uint8_t		j = sensor_manager.get_available_sensors(),
            k;

  k = j;
  for ( uint8_t i = 0; i < 6; i++ ) {
    if ( !( k & 1 ))
      strncat( unavailable_sensors, sensor_name[i], 12 );
    k >>= 1;
  }

  if ( debug_mode ) {
    Serial.print( unavailable_sensors );
    if ( j == ALL_SENSORS )
      Serial.println( "none" );
  }

  if ( j != ALL_SENSORS )
    send_alarm( "Unavailable sensors report", unavailable_sensors );
}


void AstroWeatherStation::send_alarm( const char *subject, const char *message )
{
  DynamicJsonDocument content( 512 );
  char jsonString[ 600 ];
  content["subject"] = subject;
  content["message"] = message;

  serializeJson( content, jsonString );
  post_content( "alarm.php", jsonString );
}

void AstroWeatherStation::send_data( void )
{
  get_json_sensor_data();

  if ( debug_mode )
    Serial.printf( "Sending JSON: %s\n", json_sensor_data );

  post_content( "newData.php",  json_sensor_data );
}

void AstroWeatherStation::send_rain_event_alarm( uint8_t rain_intensity )
{
  const char	intensity_str[7][13] = { "Rain drops", "Very light", "Medium light", "Medium", "Medium heavy", "Heavy", "Violent" };  // As described in RG-9 protocol
  char		msg[32];

  snprintf( msg, 32, "RAIN! Level = %s", intensity_str[ rain_intensity ] );
  send_alarm( "Rain event", msg );
}

bool AstroWeatherStation::shutdown_wifi( void )
{
  if ( debug_mode )
    Serial.printf( "[DEBUG] Shutting down WIFI.\n" );

  if ( !stop_hotspot() || !disconnect_from_wifi() )
    return false;

  WiFi.mode( WIFI_OFF );
  return true;
}

bool AstroWeatherStation::start_config_server( void )
{
  server = new AWSWebServer();
  server->initialise( debug_mode );
  return true;
}

bool AstroWeatherStation::start_hotspot()
{
  const char	*ssid		= config->get_ap_ssid(),
                *password	= config->get_wifi_ap_password();

  if ( debug_mode )
    Serial.printf( "[DEBUG] Trying to start AP on SSID [%s] with password [%s]\n", ssid, password );

  if ( WiFi.softAP( ssid, password )) {

    const IPAddress my_IP( 192, 168, 168, 1 );
    const IPAddress gateway( 0, 0, 0, 0 );
    const IPAddress subnet( 255, 255, 255, 0 );
    WiFi.softAPConfig( my_IP, gateway, subnet );
    Serial.printf( "[INFO] Started hotspot on SSID [%s/%s] and configuration server @ IP=192.168.168.1/24\n", ssid, password );
    return true;
  }
  return false;
}

bool AstroWeatherStation::setup_pref_iface( void )
{
//  esp_eth_handle_t *ethhandle;

  switch ( config->get_pref_iface() ) {

    case wifi_ap:
    case wifi_sta:
      if ( initialise_wifi() ) {
        delete( server );
        delete( config );
        alpaca->stop_discovery();
 //       ethhandle = ETH.get_eth_handle();
  //      if ( ESP_OK != esp_eth_stop( *ethhandle ) )
 //         Serial.printf( "[ERROR] Failed to stop ethernet.\n");
 //       else
          return true;
      }
      break;
    default:
      if ( initialise_ethernet() && shutdown_wifi() )
        return true;
      break;
  }
  return false;
}

bool AstroWeatherStation::startup_sanity_check( void )
{
  switch ( config->get_pref_iface() ) {
    case wifi_sta:
      return Ping.ping( WiFi.gatewayIP() );
    case eth:
      return Ping.ping( ETH.gatewayIP() );
    case wifi_ap:
      return true;
  }
  return false;
}

bool AstroWeatherStation::stop_hotspot( void )
{
  return WiFi.softAPdisconnect();
}

bool AstroWeatherStation::update_config( JsonVariant &proposed_config )
{
  return config->save_runtime_configuration( proposed_config );
}

void AstroWeatherStation::wakeup_reason_to_string( esp_sleep_wakeup_cause_t wakeup_reason, char *wakeup_string )
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
