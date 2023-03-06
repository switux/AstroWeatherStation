# AstroWeatherStation
ESP32 based weather station with astronomical observatory features

## GOAL

This project aims to provide all the instructions that are needed to build a weather station for remote astronomical observatories. On top of the usual readings, rain events and cloud coverage are reported to allow the safe operation of the observatory.

## STATUS & DEVELOPMENT

This is version 0.9 (prototype) of the project.
It is however usable in production, the things that will be improved in v1.0 are:

  - Hardware
    - Single universal PCB
    - Maybe replace the 2 channels relay by a set of mosfets to further reduce current drain (as the relay itself needs to be powered up), not really needed but for the challenge.

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

## Pricing

All parts can be found on aliexpress, except for the rain sensor and the PVC sheet which is much cheaper when locally sourced. I estimated the raw cost of materials (price vary ...) to be **about 210.- CHF VAT incl.** with about 50% spent on the RG-9 and the wind sensors (shipping is quite expensive for the latter). About 50.- are for enclosures and the other sensors, the rest well ... is the rest :-)

## ASSUMPTIONS

A Wifi network is available to send sensor data and send alarms. In my case this is provided by a 4G module attached to a Raspberry Pi 4 which pilots the instruments.

## SPECIFICATIONS

  - Power consumption: N/A (collecting data)
  - Autonomy: N/A but I think at least a couple of weeks (will cover the panel when the above point is done, and see how long it will survive as there is always a gap between theory and practice)
  - Measures (from sensor specs)
    - Illuminance range: 0-88k Lux ( up to ~730 W/m² )
    - Temperature range: -40°C to +85°C
    - Pressure: 300 to 1100 hPa
    - Wind speed: 0 to 30 m/s

## Data format

The station sends the data to a web server as a JSON string:

{
  "battery_level":92,
  "timestamp":1675767261,
  "rain_event":0,
  "temp":16.84000015,
  "pres":941.1413574,
  "rh":27.296875,
  "lux":68181,
  "ambient":21.42998695,
  "sky":-3.19000864,
  "direction":45,
  "speed":0,
  "rain":0,
  "sensors":63
}

Where

- battery_level: in % of 4.2V
- rain_event: 1 if awakened by the sensor
- temp: in °C
- pres: in hPa (QFE)
- rh: relative humidity in %
- lux: solar illuminance
- ambient: IR sensor ambient temperature
- sky: IR sensor sky temperature (substract ambient to get sky temperature, if below -20° --> sky is clear)
- direction: wind direction 0=N, 45=NE, 90=E, 135=SE, 180=W, 225=SW, 270=W, 315 NW
- speed: in m/s
- rain: 0=None, 1=Rain drops, 2=Very light, 3=Medium light, 4=Medium, 5=Medium heavy, 6=Heavy, 7=Violent
- sensors: available sensors (see source code)

## Tips

- I use 24AWG single core cable to create the tracks on the PCB (you can of course have your own custom PCB made).
- I use female pin headers for the MCU and other boards. I fried a couple of mosfet's during prototyping (even though I have a 15W iron and I am pretty careful) and lost quite some time to troubleshoot the battery level part so I decided to use pin headers too :-)
- I solder the KF2510 terminals to the wires, maybe it is me or my crimping tool but I kept having contact failures ...
- I cut the external antenna cable (1m is waaay too long but I could not find a shorter one) and crimped a male SMA to go with the RP-SMA pigtail

## REFERENCES

I found inspiration and solutions to problems in the following pages / posts:

  - No need to push the BOOT button on the ESP32 to upload new sketch
    - https://randomnerdtutorials.com/solved-failed-to-connect-to-esp32-timed-out-waiting-for-packet-header
  - Reading battery load level without draining it
    - https://jeelabs.org/2013/05/18/zero-power-measurement-part-2/index.html
  - To workaround the 3.3V limitation to trigger the P-FET of the above ( I used an IRF930 to drive the IRF9540 )
    - https://electronics.stackexchange.com/a/562942
  - Solar panel tilt
    - https://globalsolaratlas.info/
    
