
# AirQualitySensor Storage component

## MQTT broker

You need an MQTT broker to receive the data from the device to start.  For a raspberry PI you need:

```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get dist-upgrade
sudo apt-get install mosquitto mosquitto-clients python-mosquitto

echo "

listener 9001
protocol websockets

listener 1883" >> /etc/mosquitto/mosquitto.conf
```

Then restart the mosquitto service instance via `sudo systemctl restart mosquitto`.  

When the embedded device is running then you can confirm that the MQTT messages are flowing using `mosquitto_sub -h raspberrypi -t "aq"` on a raspberry pi.  

Note that you will likely have to set the IP address of the MQTT broker (which you just installed in the above) to the proper setting in [embedded.ino](../Embedded/embedded.ino).

## Air quality persistent storage

The embedded device is broadcasting on the `aq` MQTT channel.  We want to support storage of this data long-term to allow for historical analysis.  Sqllite3 is the perfect solution here as it does not require the maintenance and configuration of an RDBM instance.  We don't need users and passwords and permissions and a lot of the other things that you would get with something like Mysql. 

All data is stored within the file `air_quality.db`.  Use this to install the dependencies.

```
sudo apt update --allow-releaseinfo-change
sudo apt-get install sqlite3
sudo pip3 install sqlite3
```

Then create the sqlite database via `create_sqlite_db.py`.  You can use sqlite3 command line application for adhoc access to the data.  `sqlite3 air_quality.db` in this directory to access the DB instance.  Here is an example query to see a view of the data.

```
select datetime(ts,'unixepoch','localtime'),pm1,pm10 from aq limit 10;
```

# Copying data from MQTT into SQLlite3

The final part to the storage is to setup a mechanism to copy the data from the MQTT broker into the sqlite3 database.  First install MQTT for Python3.

```
sudo pip3 install paho.mqtt
```

Use the `./install.sh` script to create and start a service that will automatically copy all data into the DB.

