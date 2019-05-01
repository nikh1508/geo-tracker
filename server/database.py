import mysql.connector
import datetime
import logging

log = logging.getLogger(__name__)
connected = False
db = 'GeoTracker'

try:
    log.info('Connecting to Mysql server')
    mydb = mysql.connector.connect(
        host='localhost',
        user='root',
        passwd='sql007',
    )
    log.info('Mysql server connected')
    connected = True
    cursor = mydb.cursor()
    try:
        cursor.execute('USE {}'.format(db))
    except mysql.connector.Error as err:
        log.error('Database does not exist. Creating database {}'.format(db))
        cursor.execute('CREATE DATABASE {}'.format(db))
        cursor.execute('USE {}'.format(db))

except mysql.connector.Error as err:
    log.error('Could not connect to Mysql server. Error : {}'.format(err))

def table_exists(table_name):
    cursor.execute('SHOW TABLES LIKE \'{}\''.format(table_name))
    res = cursor.fetchall()
    return len(res) > 0
    
def create_table(dev_id):
    table = ('CREATE TABLE dev_{}('
            'sr_no INT AUTO_INCREMENT PRIMARY KEY,'
            'latitude DECIMAL(10, 6),'
            'longitude DECIMAL(11, 6),'
            'sat_count INT,'
            'timestamp TIMESTAMP)'.format(dev_id)
    )
    try:
        log.info('Creating table for {}'.format(dev_id))
        cursor.execute(table)
    except mysql.connector.Error as err:
        log.error('Error : {}'.format(err))

def insert(dev_id, content):
    log.info('Inserting data for {}'.format(dev_id))
    if not table_exists('dev_{}'.format(dev_id)):
        create_table(dev_id)
    row = ('INSERT INTO dev_{}(sr_no, latitude, longitude, sat_count, timestamp)'
        'VALUES(NULL, %s, %s, %s, CURRENT_TIMESTAMP)'.format(dev_id)
    )
    val = tuple(content[key] for key in sorted(content.keys()))
    try:
        cursor.execute(row, val)
        mydb.commit()
    except mysql.connector.Error as err:
        mydb.rollback()
        log.error('Could not insert into the table. Error : {}'.format(err))

def get_latest_data(dev_id):
    try:
        cursor.execute('SELECT * FROM dev_{} ORDER BY timestamp DESC LIMIT 1'.format(dev_id))
    except mysql.connector.Error as err:
        log.error('Could not fetch latest data for dev_{}. Error : {}'.format(dev_id, err))
    res = cursor.fetchall()
    if res != None:
        lat, lng, sat_count, last_update = res[0][1:]
        lat, lng = float(lat), float(lng)
        data = {}
        delta = datetime.datetime.now() - last_update
        days, hours, minutes, seconds = delta.days, delta.seconds // 3600, (delta.seconds//60) % 60, delta.seconds %60
        data['Last Updated'] =  (("{} days ".format(days) if days > 0 else "") + 
                                ("{} hrs ".format(hours) if hours > 0 or days > 0 else "") +
                                ("{} min ".format(minutes) if minutes > 0 or days > 0 or hours > 0 else "") + 
                                ("{} sec ".format(seconds)))
        data['Latitude'] = float(int(lat / 100.)) + (lat % 100.) / 60.      # conversion from ddmm.mmmmmm to decimal degrees
        data['Longitude'] = float(int(lng / 100.)) + (lng % 100.) / 60.     # conversion from dddmm.mmmmmm to decimal degrees
        data['Satellite Count'] = sat_count
        return data
    else:
        log.error('Empty dataset received from dev_{}'.format(dev_id))
        return None

def close():
    log.info('closing mysql connection')
    cursor.close()
    mydb.close()
