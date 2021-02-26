
# AirQualitySensor Storage component

The embedded device is broadcasting on an MQTT channel the air quality data seen.  We want to support storage of this data long-term to allow for historical analysis.  Sqllite3 is the perfect solution here as it does not require the maintenance and configuration of an RDBM instance.  We don't need users and passwords and permissions and a lot of the other things that you would get with something like Mysql. 

All data is stored within the file `air_quality.db`.  While sqllite3 is part of the normal distro, it is useful to have a command line access to inspect the db file.  

```
sudo apt update --allow-releaseinfo-change
sudo apt-get install sqlite3
```

You can use `sqlite3 air_quality.db` in this directory to access the DB instance.  Use something like:

```
select datetime(ts,'unixepoch','localtime'),pm1,pm10 from aq limit 10;
```

To see the data.

# Copying data from MQTT into SQLlite3

First install MQTT for Python3.

```
sudo pip3 install paho.mqtt
```

Use the `./install.sh` script to create and start a service that will automatically copy all data into the DB.

