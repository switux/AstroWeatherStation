/*
	AstroWeatherStation.h

	(c) 2023 F.Lesage

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

// MOSTFET SWITCHES
#define GPIO_ENABLE_3_3V	GPIO_NUM_5
#define GPIO_ENABLE_12V		GPIO_NUM_18

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
#define US_SLEEP		5 * 60 * 1000000				// 5 minutes
#define US_HIBERNATE	30 * 24 * 60 * 60 * 1000000ULL	// 30 days

#define MLX_SENSOR			0x01
#define TSL_SENSOR			0x02
#define BME_SENSOR			0x04
#define WIND_VANE_SENSOR	0x08
#define ANEMOMETER_SENSOR	0x10
#define RG9_SENSOR			0x20
#define ALL_SENSORS			( MLX_SENSOR | TSL_SENSOR | BME_SENSOR | WIND_VANE_SENSOR | ANEMOMETER_SENSOR | RG9_SENSOR )

// Runtime configuration
#define	GPIO_CONFIG_MODE		GPIO_NUM_4
#define FORMAT_SPIFFS_IF_FAILED true
#define	CONFIG_SSID				"AstroWeatherStation"
#define	CONFIG_SSID_PASSWORD	"AWS2023!"

// Default configuration settings
#define	SERVER		"www.datamancers.net"
#define	URL_PATH	"weather"

#define	TZNAME	"CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"
#define	MSAS_CORRECTION "-0.55"

// Datamancers.net ROOT CA
#define ROOT_CA \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----\n"

// DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING!

const char *configuration_items[] = { "ssid", "password", "server", "url_path", "tzname", "root_ca", "msas_calibration_offset" };
const char *default_configuration[] = { CONFIG_SSID, CONFIG_SSID_PASSWORD, SERVER, URL_PATH, TZNAME, ROOT_CA, MSAS_CORRECTION };

struct anemometer_t {

	SoftwareSerial	*device;
	char			*version;
	byte			request_cmd[8];
	uint16_t		speed;
};

struct wind_vane_t {

	SoftwareSerial	*device;
	char			*version;
	byte			request_cmd[8];
	uint16_t		speed;
};




char *build_config_json_string( void );
float ch0_temperature_factor( float );
float ch1_temperature_factor( float );
void change_gain( Adafruit_TSL2591 *, byte, tsl2591Gain_t * );
void change_integration_time( Adafruit_TSL2591 *, byte, tsl2591IntegrationTime_t * );
void check_ota_updates( char *, byte );
byte connect_to_wifi( JsonObject &, byte );
void deep_sleep();
void displayBanner( JsonObject &, Preferences &, char * );
void enter_config_mode( void );
float get_battery_level( byte );
const char *get_config_parameter( const JsonObject &, const char * );
bool get_hw_config( Preferences &, char *, anemometer_t *, wind_vane_t * );
void hibernate();
void loop( void );
bool read_runtime_config( JsonObject *, byte );
void retrieve_sensor_data( JsonObject &, JsonDocument&, Adafruit_BME280*, Adafruit_MLX90614*, Adafruit_TSL2591*, anemometer_t*, wind_vane_t*, HardwareSerial*, byte, byte, byte *, byte, byte * );
void send_alarm( const JsonObject &, const char *, const char *, byte );
void initialise_anemometer( anemometer_t * );
void initialise_BME( Adafruit_BME280 *, byte *, byte );
void initialise_MLX( Adafruit_MLX90614 *, byte *, byte );
void initialise_RG9( HardwareSerial *, JsonObject &, byte *, byte, byte );
void initialise_sensors( JsonObject &, Adafruit_BME280*, Adafruit_MLX90614*, Adafruit_TSL2591*, anemometer_t*, wind_vane_t*, HardwareSerial*, byte *, byte, byte );
void initialise_TSL( Adafruit_TSL2591 *, byte *, byte );
void initialise_wind_vane( wind_vane_t * );
void OTA_callback( int, int );
const char *OTA_message( int );
void post_content( const JsonObject &, const char *, const char *, byte );
void print_config( const JsonObject & );
float read_anemometer( anemometer_t *, byte *, byte );
void read_BME( Adafruit_BME280 *, float *, float *, float *, byte, byte );
void read_MLX( Adafruit_MLX90614 *, float *, float *, byte, byte );
byte read_RG9( HardwareSerial *, byte );
const JsonObject& read_runtime_config( bool *, byte );
void read_SQM( Adafruit_TSL2591 *, byte, byte, float, float, float *, float *, uint16_t *, uint16_t *, uint16_t *, uint16_t * );
int read_TSL( Adafruit_TSL2591 *, byte, byte );
int read_wind_vane( wind_vane_t *, byte *, byte );
void reboot( void );
void report_unavailable_sensors( JsonObject &, byte, byte );
void reset_config_parameter( void );
void retrieve_sensor_data( JsonObject &, JsonDocument& , Adafruit_BME280 *, Adafruit_MLX90614 *, Adafruit_TSL2591 *, anemometer_t *, wind_vane_t *, HardwareSerial *, byte, byte, byte *, byte, byte * );
byte RG9_read_string( HardwareSerial *, char *, byte );
const char *RG9_reset_cause( char );
const char *save_configuration( const char * );
void send_alarm( const JsonObject &, const char *, const char *, byte );
void send_data( const JsonObject&, const JsonDocument&, byte );
void send_rain_event_alarm( JsonObject &, byte, byte );
void set_configuration( void );
void setup( void );
byte SQM_get_msas_nelm( Adafruit_TSL2591 *, byte, float, float, float *, float *, uint16_t *, uint16_t *, uint16_t *, uint16_t * );
byte SQM_read_with_extended_integration_time( Adafruit_TSL2591 *, byte, float, uint16_t *, uint16_t *, uint16_t * );
bool start_hotspot( void );
void wakeup_reason_to_string( esp_sleep_wakeup_cause_t , char * );
void web_server_not_found( void );
