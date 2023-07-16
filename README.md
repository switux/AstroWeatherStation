# AstroWeatherStation

ESP32 based weather station with astronomical observatory features

## GOAL

This project aims to provide all the instructions that are needed to build a weather station for remote astronomical observatories. On top of the usual readings, rain events and cloud coverage are reported to allow the safe operation of the observatory.

## STATUS & DEVELOPMENT

This is version 1.0 of the project. You may want to have a look at version 2.0 even though it is still being tested (stability/robustness).

Improvements since v0.9:

  - Hardware
    - Single universal PCB
    - RG-9 has now a 12V power input, aligned with the wind sensors, because of brownouts with the 5V output (unstabilitiy with the Mini560, too close from RG-9 voltage requirements)
    - 5V relay is now a 12V relay and has been moved between the wind sensors' power input and the MT3608 output, because of the above RG9 issue
    - Using a latching push button for debug mode
    - Better voltage divider resistor values to be closer to the 3.3V max voltage while still using mostly "mainstream" resistor values
    
  - Software
    - More robust RG-9 probing (although rain events are correctly reported)
    - Battery level estimation improvements, shrunk voltage range from 0V-4.2V to 3V-4.2V to take battery's EODV into account
    - Alarms sent in case the RG-9 has firmware issues reported during its initialisation
    
Possible future improvements:

  - Hardware
    - Maybe replace the 2 channels relay by a set of mosfets to further reduce current drain (as the relay itself needs to be powered up)
    
  - Software
    - Some runtime configuration parameters can be retrieved over HTTPS from the observatory's server (to avoid rebuilds)
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
  
  - Alarms for:
  
    - Sensor unavailability
    - Rain event
    - Low battery
    - Rain sensor HW problem
  
  - Operations:

    - External reboot button
    - Debug button (to be pushed when rebooting to activate debug mode)
    - External micro USB socket for debugging (serial console) and firmware updates

## ASSUMPTIONS

A Wifi network is available to send sensor data and send alarms. In my case this is provided by a 4G module attached to a Raspberry Pi 4 which pilots the instruments.

## SPECIFICATIONS

  - Power consumption: N/A
  - Autonomy: N/A
  - Batteries max. charges / life span: **500x or 2-3 years**
  - Measures
    - Illuminance range: 0-88k Lux ( up to ~730 W/m² )
    - Temperature range: -40°C to +85°C
    - Pressure: 300 to 1100 hPa
    - Wind speed: 0 to 30 m/s

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

I found inspiration in the following pages / posts:

  - No need to push the BOOT button on the ESP32 to upload new sketch
    - https://randomnerdtutorials.com/solved-failed-to-connect-to-esp32-timed-out-waiting-for-packet-header
  - Reading battery load level without draining it
    - https://jeelabs.org/2013/05/18/zero-power-measurement-part-2/index.html
  - To workaround the 3.3V limitation to trigger the P-FET of the above ( I used an IRF930 to drive the IRF9540 )
    - https://electronics.stackexchange.com/a/562942
