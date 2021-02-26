#!/usr/bin/python3

# This program will listen for incoming http requests and 
# serve data that is extracted from a sqlite3 database instance. 

import sqlite3
import time

from http.server import BaseHTTPRequestHandler, HTTPServer
from functools import partial
import logging
import urllib.parse
import argparse
import socket
import json

logging.basicConfig(level=logging.DEBUG, format='(%(threadName)-10s) %(message)s')

parser = argparse.ArgumentParser(description='Server for AirQuality project')
parser.add_argument('--port', type=int, help='Port to listen on', default=9000)
config = parser.parse_args()

db_file = "/home/pi/AirQualitySensor/storage/air_quality.db"

# Return the primary IP address for this box.  Useful in 
# logging output.
def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

class MyServer(BaseHTTPRequestHandler):
    def do_GET(self):
        logging.debug("received a GET request; path: %s", self.path)
        try:
            if self.path == "/":
                self.showMainMenu()
            elif self.path == ("/data"):
                self.showGetDataUI()
            elif self.path.startswith("/data?"):
                args = urllib.parse.parse_qs(self.path[6:])
                logging.debug("received data request; args: %s", str(args))
                self.getData(args["DATE"][0], args["AGGREGATION"][0])
            else:
                logging.debug("Unexpected request")
                self.send_error(404,'File Not Found: %s' % self.path)
        except Exception as e:
            logging.exception("Error in handling request", e)
            self.send_error(404,'Error in handling request: %s' % str(e))

        logging.debug("Done with request")

    def send(self, s):
        self.wfile.write(bytes(s,"utf-8"))

    def showMainMenu(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.send("<html>")

        self.send("<h1>AirQuality Server</h1>")
        self.send("<a href=\"/data\">Fetch data</a>")
        self.send("</html>")

    def showGetDataUI(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.send("<html>")

        currentTime = int(time.time())
        defaultTime = int( currentTime / 86400 ) * 86400

        self.send("<h1>Fetch data</h1>")
        self.send("<form method=\"get\" action=\"data\">")
        self.send("Date (as unix timestamp, start of day): <input size=\"9\" type=\"text\" name=\"DATE\" value=\"{}\"></BR>".format(defaultTime))
        self.send("Aggregation: <input size=\"6\" type=\"text\" name=\"AGGREGATION\"></BR><BR>")
        self.send("<input type=\"submit\" value=\"Submit\"></form>")
        self.send("</html>")

    def getData(self,date,aggregation):
        conn = sqlite3.connect(db_file)
        
        logging.debug("Getting data; date: %s, aggregation: %s, dateAsInt: %d", date, aggregation, int(date))
        get_data_sql = """ 
            SELECT (round(ts / {agg},0) * {agg}) interval,
                    min(pm10) minimum,
                    max(pm10) maximum,
                    avg(pm10) average
            FROM aq
            WHERE ts > {min_time} and ts < {max_time}
            GROUP BY interval
            ORDER BY interval
            """.format(agg = aggregation, min_time = int(date), max_time = int(date) + 86400)
        logging.debug("Getting data; sql: %s", get_data_sql)

        cursor = None
        try: 
            cursor = conn.cursor()
            cursor.execute(get_data_sql)

            # fetchall?  Sure, what could go wrong?!?
            rows = cursor.fetchall()
            json_string = json.dumps(rows)
            
            logging.debug("returning results; count: %d", len(rows))

            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.send(json_string)

        except sqlite3.Error as e:
            logging.error( "Error getting data; sql: %s error: %s", get_data_sql, str(e))
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.send("Error! sql: {}, error: {}".format(get_data_sql, str(e)))

if __name__ == "__main__":
    webServer = HTTPServer(('', config.port), MyServer)
    print("Server started http://%s:%s" % (get_ip(), config.port))

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")
