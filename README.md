# AstroWeatherStation
ESP32 based weather station with astronomical observatory features

## GOAL

This project aims to provide all the instructions that are needed to build a weather station for remote astronomical observatories. On top of the usual readings, rain events and cloud coverage are reported to allow the safe operation of the observatory.

## STATUS & DEVELOPMENT

This is version 0.9 (prototype) of the project.
It is however usable in production, the things that will be improved in v1.0 are:

  - Hardware
    - Single universal PCB
    - Maybe replace the 2 channels relay by a set of mosfets to further reduce current drain (as the relay itself needs to be powered up)

  - Software
    - Some runtime configuration parameters can be retrieved over HTTPS from the observatory's server (to avoid rebuilds)
    - More robust RG-9 probing (although rain events are correctly reported)
    - Stay in debug mode until the next reboot
  
## FEATURES

  - Weather parameters:
  
    - Temperature
    - Pressure
    - Relative humidity
    - Wind speed
    - Wind direction
    - Rain detection
    - Rain intensity
  
  - Astronomical parameters:
  
    - Cloud coverage
    - Solar irradiance
  
  - Mail alarms for:
  
    - Sensor unavailability
    - Rain event
  
  - Operations:

    - External reboot button
    - Debug button (to be pushed when rebooting to activate debug mode)
    - External micro USB socket for debugging (serial console) and firmware updates

## ASSUMPTIONS

A Wifi network is available to send sensor data and send alarms. In my case this is provided by a 4G module attached to a Raspberry Pi 4 which pilots the instruments.

## SPECIFICATIONS

  - Power consumption: N/A
  - Autonomy: N/A
  - Measures
    - Illuminance range: 0-88k Lux ( up to ~730 W/m² )
    - Temperature range: -40°C to +85°C
    - Pressure: 300 to 1100 hPa
    - Wind speed: 0 to 30 m/s

## REFERENCES

I found inspiration in the following pages / posts:

  - No need to push the BOOT button on the ESP32 to upload new sketch
    - https://randomnerdtutorials.com/solved-failed-to-connect-to-esp32-timed-out-waiting-for-packet-header
  - Reading battery load level without draining it
    - https://jeelabs.org/2013/05/18/zero-power-measurement-part-2/index.html
  - To workaround the 3.3V limitation to trigger the P-FET of the above ( I used an IRF930 to drive the IRF9540 )
    - https://electronics.stackexchange.com/a/562942
