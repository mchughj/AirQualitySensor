
# AirQualitySensor server component

It is necessary to have a server instance that allows for http access to the historical data.  This is used by the date-based graphs that are vended by the embedded device itself.  

In order for this to work you must first follow the instructions in the README file found in the [storage](../storage/README.md) file.  

Once the DB instance exists and data is being automatically copied from the MQTT channel into the database then run `./install.sh` to create and start a service that will serve http requests for historical data.  The webserver will be run on port 9000 by default.

