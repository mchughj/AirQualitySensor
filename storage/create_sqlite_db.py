#!/usr/bin/python3

# This program will create the table structure within the 
# sqlite3 database instance.  It destroys existing data 
# but only if you allow it.

import sqlite3
import os.path

def create_connection(db_file):
    """ create a database connection to a SQLite database """
    conn = None
    try:
        conn = sqlite3.connect(db_file)
        print("Created the database '{}' with sqlite version {}".format(db_file,sqlite3.version))
    except Error as e:
        print(e)
    return conn

def drop_then_create_table(conn):
    try:
        c = conn.cursor()
        remove_table_sql = """ DROP TABLE IF EXISTS aq;""" 
        c.execute(remove_table_sql)
        create_table_sql = """ CREATE TABLE IF NOT EXISTS aq( 
                                  id integer PRIMARY KEY,
                                  sensor_id integer,
                                  ts integer,
                                  pm1 integer,
                                  pm10 integer
                                );""" 
        c.execute(create_table_sql)
    except sqlite3.Error as e:
        print(e)

conn = None
database_name = 'air_quality.db'
if os.path.exists(database_name):
    while True:
        answer = raw_input(
                "DB {} exists!  Continue will erase all data.  Continue? [y/n]: ".format(database_name))
        if answer.lower() not in ('y', 'n'):
            print("Not an appropriate choice.")
            continue
        if answer.lower() == 'y':
            break
        else:
            exit()
try:
    conn = create_connection(database_name)
    drop_then_create_table(conn)
finally:
    if conn:
        conn.close()

