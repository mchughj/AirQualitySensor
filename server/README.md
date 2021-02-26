
# AirQualitySensor server component

It is necessary to have a server instance that allows for http access to the historical data.  This is used by the date-based graphs that are vended by the embedded device itself.  

In order for this to work you must run this first.  (You only need to do this once of course.)

```
cd ../storage
./create_sqlite_db.py
./install.sh
```

Once the DB instance exists and any MQTT is stored with the database.  Once this has been completed use `./install.sh` to create and start a service that will serve the requests.  The webserver will be run on port 9000 by default.

