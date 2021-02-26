# AirQualitySensor Project
I've become highly allergic to wood dust so this is part of my efforts to manage that.  This project tracks the harmful air quality found in my wood workshop.  

This project utilizes an [Adafruit Feather Huzzah](https://www.adafruit.com/product/2821) along with a [small OLED display](https://www.adafruit.com/product/2900).  To setup the feather Huzzah see [Adafruit's helpful guide](https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide).  The feather is connected to a [PMS5003](https://www.adafruit.com/product/3686).

Dependent embedded libraries can be found in dependency directory as zips.
* PMS air quality client library - https://github.com/riverscn/pmsx003
* WifiManager - https://github.com/tzapu/WiFiManager
* AdaFruit GFX Library - https://github.com/adafruit/Adafruit-GFX-Library
* AdaFruit SSD1306 Library - https://github.com/adafruit/Adafruit_SSD1306
* PubSubClient for MQTT - https://github.com/knolleary/pubsubclient

By itself the embedded project will show the current air quality metrics with frequent updates.  This isn't enough for me though as I need access to historical data so that when I do have an alergic reaction I can look back at the air quality within my space and see if patterns emerge.  

The embedded device publishes MQTT data to a broker defined within the `MQTT_SERVER` in [Embedded.ino](Embedded/Embedded.ino).  A storage component, which can be systemd managed via [install.sh](storage/install.sh) will copy the MQTT data into a SQLite3 instance (which is created by [create_sqlite_db.py](storage/create_sqlite_db.py).  Finally a server-side component provides http access to the historical data and can be systemd managed via [install.sh](server/install.sh).  See the README files within the storage and server folders for details on getting these components installed and running.  

Finally, the embedded device has its own web server which allows for pretty views into both real-time and historical data.  These views require all components to be installed and running to work properly. 

## Generic Information

A useful resource about wood dust can be found [here](http://www.fwwa.org.au/Art005_WoodDust_c1.pdf).

This section contains distilled information from that, along with other, sources.

A micron is 1x10-6 meters.  Or 1 micron is 0.001 millimeters.  A human hair is about 70 microns or 0.07mm.  

Wood dust smaller than 0.1 microns is considered 'safe' in that it is so light it will be suspended in the air and breathed in and out easily.  Not sure if I buy that but perhaps from a lung cancer perspective it is true.

The PMS5003, the device used in this build, uses "03" to capture the minimum distinguishable particles.  So this sensor can only detect particles 0.3microns or larger.

Particles larger than 10 microns do not remain suspended in the air for long.  So it is the particles from 10 microns to 0.1 microns that people are most worried about.

Most people who are less than 50 years old cannot see a particle smaller than 10 microns.  So everything we see is larger than that.

The label PM refers to "Particle Matter".  So PM10 means all particles that are 10 microns or smaller.  (Modulo the threshold for which the sensor can detect, of course.  If the device says 1,000 PM10 and it is a PM5003 then it is the count of particles from 10 microns down to 0.3 microns.)

### Standards

So what is a safe amount of particle matter in the air?  That would absolutely depend on the type of particles, of course, but for my use case I'm concerned about wood dust particles.  One standard says that 5 milligrams of dust in one cubic meter of air - which is 5 ppm - 5 parts per meter - for soft woods and 1 ppm for hardwoods is the maximum allowable reading.

A PPM is a particle per cubic meter which is equivalent to mg/m^3.  Or milligram per cubic meter.

### Problems with simple standards

The standard generic 'PPM' doesn't specify the size of the wood dust which is the primary concern.  If you had 0.3g of soft wood dust on your shirt (this is easy and common when cutting wood) then this would be enough dust suspended in the air (by weight and independent of the size per this measure) to contaminate a large space.  0.3g / 0.005g = 60 cubic meters of space (which is equal to 2118 cubic feet of space).  If you had a wood workshop which was 21' x 10' x 10' = 2100 cubic feet and you introduced that much dust it would be beyond the allowed limit.  But, in this example, this is just wood dust on your shirt and is likely large enough to be seen and doesn't pose a real threat to your lunges.  

So looking at pure PPM or particles per cubic meter isn't useful enough.

### A Better standard

European standards capture the size of the wood dust and not just the weight.  It is the small dust particles in the air that are breathed in which impact us the most.  So the concentratioin of PM10 - which presumably is in the air after wood working operations - relative to the overall volume of space.

They say that safety standards require less than 0.1 ppm for _PM10 airborne dust_.  

So, if you were in a 300 square foot shop (this is the size of my basement) and the ceilings are 7 feet tall (they are probably about this) then this is 2100 cubic feet of space.  

A reading of 0.1 ppm * 60 cubic meters = 0.6ppm of PM10 is unsafe in my basement.

## Units

Within the literature, and on devices, common units are:

 * Micrograms per cubic meter: μg/m3 
 * Milligrams per cubic meter: mg/m3

1000 μg/m3 = 1 mg/m3.  

## The PMS5003 Sensor

The PMS5003 sensor used here has a datasheet which can be found here:
  http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf?sfvrsn=2

There are these data elements:

1. PM1.0 μg/m3 - CF=1
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

### What is safe for me?

I want to pay most attention to PM10 μg/m3 - CF=1 -- or data point #3.  
It is μg/m3 and not mg/m3 so, per the European standard, for my environment 0.6ppm is considered unsafe.  0.6ppm == 600 μg/m3.  So when data point #3 goes about 600 then it should be considered unsafe.  

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

