#!/usr/bin/python3

# This program will listen to an MQTT broker for message on the topic 'aq'
# and insert them into the sqlite3 database instance constructed in the 
# other script within this directory.

import argparse
import sqlite3
import paho.mqtt.client as mqtt
import time

parser = argparse.ArgumentParser(description='Storage populator')
parser.add_argument('--db', type=str, default='air_quality.db', help='DB file to open')
config = parser.parse_args()

mqtt_broker = "localhost"
mqtt_port = 1883
mqtt_keep_alive_interval = 45
mqtt_topic = "aq"

# Subscribe to sensor data
def on_connect(client, userdata, flags, rc):
    client.subscribe(mqtt_topic, 0)

def on_message(client, obj, msg):
    # Data is stored within msg.payload
    sql = "INSERT INTO aq(sensor_id,ts,pm1,pm10) VALUES(?,?,?,?)"
    parts = msg.payload.split(b",")
    row_data = (1, time.time(), int(parts[0]), int(parts[2]))
    try:
        cur = conn.cursor()
        cur.execute(sql, row_data)
        conn.commit()
    except sqlite3.Error as e:
        print("Error executing SQL statement: {}".format(e))
    print("Added data: {}".format(row_data))

print("Going to connect to the database '{}'".format(config.db))
conn = sqlite3.connect(config.db)
print("Connected to the database '{}' with sqlite version {}".format(config.db,sqlite3.version))

mqttc = mqtt.Client()

# Assign event callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect

# Connect
mqttc.connect(mqtt_broker, int(mqtt_port), int(mqtt_keep_alive_interval))

print("Conencted to mqtt")

# Run forever allowing MQTT to do its job.
try:
    mqttc.loop_forever()
finally:
    conn.close()

