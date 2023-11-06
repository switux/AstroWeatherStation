# AstroWeatherStation

ESP32 based weather station with astronomical observatory features

## GOAL

This project aims to provide all the instructions that are needed to build a weather station for remote astronomical observatories. On top of the usual readings, rain events and cloud coverage are reported to allow the safe operation of the observatory.

## STATUS & DEVELOPMENT

This is version 2.0-RC of the project. **Do not update a 1.x hardware design with a 2.0 firmware.**

Stability tests are underway.
See it in action @ https://www.datamancers.net/weather

New features & improvements in this version:

  - Hardware
    - Replacement of relays by mosfets.
    - Use of logic level mosfets (IRLZ44N) along with a 3.3V to 5V level shifter (SN74AHCT125) to reach minimum Rds(on).
    - Use of LM7805 to deliver 5V to the level shifter.
    - DD04CVSA 12V instead of 5V, removal of MT3608.
  
  - Software
    - To save batteries, in case of a potential rain event, other sensors are no longer powered up and interrogated.
    - Support of different wind sensors models requiring different command messages and communication speeds.
    - OTA updates are no longer performed in case of a potential rain event (we have better things to do).
    - Using ESP chip id and revision to better identify boards for OTA updates.
    - Support of specific debug button behaviour to enter configuration mode (10s pressure)

**A 24x7 "always on" version is under development and will have the following changes:**

  - Hardware
    - Removal of solar panel support
    - Removal of DD04CVSA charger and associated batteries
    - Removal of MOSFETs and LM7805
    - Removal of battery load monitoring
    - Addition of GPS
    - Addition of PoE (and, incidently, ethernet ...)
    
  - Software
    - Disabling of battery monitoring
    - Disabling of 3.3V/12V control
    - Addition of GPS support
    - ASCOM Alpaca
    - Monitoring and configuration web server
    - NTP Server
    - Web setup UI
    - Simulatenous support of WiFi and ethernet with fallback strategies
    
The things that might be improved in v2.1 are:

  - Software
    - ASCOM Alpaca
    - Finer cloud sensor reporting: clear/cloudy/overcast instead of only clear/clouds
    - QNH pressure
    - Dew point reporting
      
  - Hardware
    - GPS
   
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
  
    - Cloud coverage (Clear/Clouds)
    - Solar irradiance
    - Sky Quality Meter (MPSAS and MELM)
  
  - Alarms (to be handled on the server side) for:
  
    - Sensor unavailability
    - Rain event
    - Low battery
    - Rain sensor HW problem
  
  - Operations:

    - External reboot button
    - Debug button (to be pushed when rebooting to activate debug mode)
    - External micro USB socket for debugging (serial console) and firmware updates
    - Configuration mode and runtime configuration updates activable if debug button is pressed 10s upon reboot

## ASSUMPTIONS

A Wifi network is available to send sensor data and send alarms. In my case this is provided by a 4G module attached to a Raspberry Pi 4 which pilots the instruments.

## SPECIFICATIONS

  - Power consumption:
    - 14mA in sleep mode
    - 40mA when active (for about 20s every 5 minutes)
  - Autonomy: measurement in progress!
  - Measures (from sensor specs)
    - Illuminance range: 0-88k Lux ( up to ~730 W/m² )
    - Temperature range: -40°C to +85°C
    - Pressure: 300 to 1100 hPa
    - Wind speed: 0 to 30 m/s

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
