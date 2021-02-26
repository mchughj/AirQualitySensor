
# AirQualitySensor embedded component

This is the software that runs on the actual device.  See comments at the top of `Embedded.ino` for details.  Note that the files currently have hard coded values for where the MQTT server instance is and where the webserver to handle historical data lives.  

The embedded device will automatically run a webserver and the IP address of the device is shown in the OLED.  Connect to see some views of the live and historical data.  If you decide to do this then make sure you run `../storage/install.sh` and `../server/install.sh`.

