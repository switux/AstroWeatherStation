# AstroWeatherStation

ESP32 based weather station with astronomical observatory features

## GOAL

This project aims to provide all the instructions that are needed to build a weather station for remote astronomical observatories. On top of the usual readings, rain events and cloud coverage are reported to allow the safe operation of the observatory.

## STATUS & DEVELOPMENT

This is version 3.0 of the project. **It is still work in progress.**

Stability tests are underway.

New features in this version:

  - Hardware
    - Addition of POE module
    - Addition of GPS support
  - Software
    - ASCOM ALPACA Server
    - Support of ultrasonic wind sensors for adverse conditions (frost)

Improvements:

  - Configuration UI
  - Removed configuration button and replaced by guard time on the reboot button (10s)
  - Clear/cloudy/overcast sky states instead of overcast/clear

The things that might be improved in v3.1 are:

  - Software
    - TBD
  - Hardware
    - TBD
   
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
    - Sky Quality Meter (still experimental, see below)
  
  - Alarms (to be handled on the server side) for:
  
    - Sensor unavailability
    - Rain event
    - Low battery
    - Rain sensor HW problem
  
  - Operations:

    - External reboot button
    - Debug button (to be pushed when rebooting to activate debug mode)
    - External micro USB socket for debugging (serial console) and firmware updates
    - Configuration mode and runtime configuration updates activable via button

## Pricing

This section is being reorganised as there are different hardware setups. All parts can be found on aliexpress, except for the rain sensor and the PVC sheet which is much cheaper when locally sourced. I estimated the raw cost of materials (price vary ...) to be **about 215.- CHF VAT incl.** with about 50% spent on the RG-9 and the wind sensors (shipping is quite expensive for the latter). About 50.- are for enclosures and the other sensors, the rest well ... is the rest :-)

## SPECIFICATIONS

This section is being reworked as there are different hardware setups.

  - Power consumption: N/A (collecting data)
  - Autonomy: N/A (collecting data)
  - Measures (from sensor specs)
    - Illuminance range: 0-88k Lux ( up to ~730 W/m² )
    - Temperature range: -40°C to +85°C
    - Pressure: 300 to 1100 hPa
    - Wind speed: 0 to 60 m/s

## About the SQM feature

This is now validated.

    - 20°FoV lens put in front of the TSL2591 (comparable to the SQM-Lx)
    - Calibration against my SQM-LE done
    
## Open points

   - The RG9 sensor raises false positives during day because (I guess) of condensation. I try to mitigate this but since it is happening during daytime, it is only a minor issue.

## Runtime configuration interface

## Alarms format

The station sends alarm to an HTTPS endpoint as a JSON string:

{
  "subject": "reason for the alarm",
  "message": "what happened"
}

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
- timestamp: Unix epoch time
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
  - TSL2591 response to temperature by Marco Gulino
    - https://github.com/gshau/SQM_TSL2591/commit/9a9ae893cad6f4f078f75384403336409ca85380  
  - Conversion of lux to W/m<sup>2</sup>
    - P. Michael, D. Johnston, W. Moreno, 2020. https://pdfs.semanticscholar.org/5d6d/ad2e803382910c8a8b0f2dd0d71e4290051a.pdf, section 5. Conclusions. The bottomline is 120 lx = 1 W/m<sup>2</sup> is the engineering rule of thumb.
