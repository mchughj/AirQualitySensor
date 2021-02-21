# AirQualitySensor
I've become highly allergic to wood dust so this is part of my efforts to manage that.  This project tracks the harmful air quality found in my wood workshop.  

This project utilizes an [Adafruit Feather Huzzah](https://www.adafruit.com/product/2821) along with a (small OLED display)[https://www.adafruit.com/product/2900].  To setup the feather Huzzah see (Adafruit's helpful guide)[https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide].  The feather is connected to a (PMS5003)[https://www.adafruit.com/product/3686].

Dependent embedded libraries can be found in dependency directory as zips.
* PMS air quality client library - https://github.com/riverscn/pmsx003
* WifiManager - https://github.com/tzapu/WiFiManager
* AdaFruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
* AdaFruit SSD1306 Library - https://github.com/adafruit/Adafruit_SSD1306
* PubSubClient for MQTT - https://github.com/knolleary/pubsubclient

## Generic Information

http://www.fwwa.org.au/Art005_WoodDust_c1.pdf

A micron is 1x10-6 meters.  Or 1 micron is 0.001 millimeters.  
A human hair is about 70 microns or 0.07mm.  

Wood dust smaller than 0.1 microns is considered 'safe' in that it is so light it will be suspended in the air and breathed in and out easily.  Not sure if I buy that but perhaps from a lung cancer perspective it is true.

(The PMS5003 uses "03" to capture the minimum distinguishable particles.  So this sensor can only detect particles 0.3microns or larger.)

Particles larger than 10 microns do not remain suspended in the air for long.  So it is the particles from 10 microns to 0.1 microns that people are most worried about.

Most people who are less than 50 years old cannot see a particle smaller than 10 microns.  So everything we see is larger than that.

The label PM refers to "Particle Matter".  So PM10 means all particles that are 10 microns or smaller.  (Modulo the threshold for which the sensor can detect.)

### Standards

One standard says that 5 milligrams of dust in one cubic meter of air - which is 5 ppm - 5 parts per meter - for soft woods and 1 ppm for hardwoods.

A PPM is a particle per cubic meter which is equivalent to mg/m^3.  Or milligram per cubic meter.

### Problems with simple standards

The standard generic 'PPM' doesn't specify the size of the wood dust which is the primary concern.  If you had 0.3 of wood dust on your shirt (this is easy and common) then this would be enough dust suspended in the air (by weight and independent of the size per this measure) to contaminate a large space.  0.3 / 0.005 = 60 cubic meters of space (which is equal to 2118 cubic feet of space).  If you had a wood workshop which was 21' x 10' x 10' = 2100 cubic feet and you introduced that much dust it would be beyond the allowed limit.

So looking at pure PPM or particles per cubic meter isn't useful enough.

### A Better standard

European standards capture the size of the wood dust and not just the weight.  It is the small dust particles in the air that are breathed in which impact us the most.  So the concentratioin of PM10 - which presumably is in the air after wood working operations - relative to the overall volume of space.

They say that safety standards require less than 0.1 ppm for PM10 airborne dust.  

So, if you were in a 300 square foot shop (my basement) and the ceilings are 7 feet tall (they are probably about this) then this is 2100 cubic feet of space.  

A reading of 0.1 ppm * 60 cubic meters = 0.6ppm of PM10 is unsafe in my basement.

## Units

Common readings are:

Micrograms per cubic meter: μg/m3 
Milligrams per cubic meter: mg/m3

1000 μg/m3 = 1 mg/m3

## The PMS5003 Sensor

The PMS5003 sensor used here has a datasheet which can be found here:
  http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf?sfvrsn=2

There are these data elements:

1.  PM1.0 μg/m3 - CF=1
2. PM2.5 μg/m3 - CF=1
3. PM10  μg/m3 - CF=1

4. PM1.0 μg/m3 - CF=atm
5. PM2.5 μg/m3 - CF=atm
6. PM10  μg/m3 - CF=atm

7. Particle count beyond 0.3 microns in 0.1L of air
8. Particle count beyond 0.5 microns in 0.1L of air
9. Particle count beyond 1.0 microns in 0.1L of air
10. Particle count beyond 2.5 microns in 0.1L of air
11. Particle count beyond 5.0 microns in 0.1L of air
12. Particle count beyond 10  microns in 0.1L of air

### What is up with CF?

The CF = 1 vs 'atm' is covered in this random article: https://www.mdpi.com/1424-8220/20/17/4796/html.  CF=1 is 'indoor' and 'atm' is outdoor.  This link is a neat article about the use of the 5003 as a low-cost method of measuring wildfire smoke.  "Low-cost" is anything < $5,000!

I want to pay most attention to PM10 μg/m3 - CF=1 -- or data point #3.  
It is μg/m3 and not mg/m3 so for my environment 0.6ppm is considered unsafe.  0.6ppm == 600 μg/m3.  Anything above that is unsafe per safety standards.

### Power Supply

Should I power the fan using 3.3V or 5V?  It appears to work for both although the numbers for 3.3V are a bit smaller.  From the datasheet:

> DC 5V power supply is needed because the FAN should be driven by 5V.

This confirmed that operating at 3.3V the fan will turn more slowly and thus the results for ppm will be off since it is assuming that the fan is pulling air across the sensor at a specific rate.  When possible use the USB (or 5V) power rail on the feather.

### Stability

> Stable data should be got at least 30 seconds after the sensor wakeup from the sleep mode because of the fan’s performance.
 
Takes a bit for the fan to spin up and the data to normalize.

### Gotchas

The shell for the PMS5003 is grounded - oh boy

> Metal shell is connected to the GND so be careful not to let it shorted with the other parts of circuit except GND.
 
Don't let the shell touch the ESP8266 I guess.

