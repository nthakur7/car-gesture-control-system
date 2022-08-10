import sys
sys.path.append('/var/jail/home/team40/detectionmodel')

import sqlite3
import datetime
import requests
import json
import base64
from model_opencv import main

modes_db = '/var/jail/home/team40/modecheck/modes.db'
imagedata_db = '/var/jail/home/team40/modecheck/imagedata.db'

def request_handler(request) :

    if(request["method"] == 'POST'):

        if(request['form']['user'] == 'model'):

            #model has posted what the new mode is
            #access this new mode through request['form']['data']
            #post new mode to modes.db

            currentMode = int(request['form']['data'])
            currentTime = datetime.datetime.now()

            conn = sqlite3.connect(modes_db)  # connect to that database (will create if it doesn't already exist)
            c = conn.cursor()  # move cursor into database (allows us to execute commands)
            c.execute('''CREATE TABLE IF NOT EXISTS modes (mode integer, timing datetime);''') # run a CREATE TABLE command if it doesn't exist

            c.execute('''INSERT INTO modes VALUES (?,?);''', (currentMode, currentTime))
            conn.commit() # commit commands
            conn.close() # close connection to database

        elif request['form']['user'] == 'arduino':
            #arduino has posted image data
            imageData = request['form']['data']
            b = bytes.fromhex(imageData)
            with open("/var/jail/home/team40/images/newimg.jpg", 'wb') as f:
                f.write(b)
                f.close()

        else:
            return 'error'
        

    # GET request
    elif (request["method"] == 'GET'):

        #arduino is trying to get latest mode detected
        #return latest mode in modes.db
        conn = sqlite3.connect(modes_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # move cursor into database (allows us to execute commands)
        c.execute('''CREATE TABLE IF NOT EXISTS modes (mode integer, timing datetime);''') # run a CREATE TABLE command if it doesn't exist

        try:
            values = c.execute('''SELECT mode FROM modes ORDER BY timing DESC;''').fetchone() #ORDER BY timing DESC
            if values: # mode stored
                conn.commit()
                conn.close()
                return values[0]
            else: # no mode stored
                return -1

        except Exception as e:
            return e
    
    else:
        return "error"

        
