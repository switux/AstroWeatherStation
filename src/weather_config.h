/*
 * weather_config.h 
 * 
 * Please customise
 *
 */

const char *ssid     = "your ssid";
const char *password = "your wifi password";

const char *server = "yourserver.org";
const char *url = "https://yourserver.org/endpoint";

const char *root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
"-----END CERTIFICATE-----\n";

#define SMTP_HOST "mailserver.org"
#define SMTP_PORT 25

#define FROM "weather@yourdomain.org"
#define SMTP_PASSWORD "foo"

#define TO "you@yourdomain.org"
