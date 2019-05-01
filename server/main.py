import logging
logging.basicConfig(filename='server.log', format='[%(levelname)-7s] : %(asctime)s : %(name)-8s : %(message)s',
                    level=logging.DEBUG, datefmt='%b %d, %g | %H:%M:%S')
log = logging.getLogger(__name__)

from flask import Flask, request, jsonify, Response, render_template
import database

app = Flask(__name__)

latest_data = {'Last Update': '25 sec ago', 'Latitude': 12.823521, 'Longitude': 80.043310,  'Satellite Count' : 12}

@app.route('/')
def index():
    return render_template('index.html', data = database.get_latest_data(666))

@app.route('/api/post_location/<dev_id>', methods=['GET', 'POST'])
def post_location(dev_id):
    content = request.json
    if all(keys in content for keys in ('lat', 'lng', 'sat_count')) and len(content) == 3:
        database.insert(dev_id, content)
        return Response(status=200)
    else:
        log.info('Improper data reveived : {}'.format(content))
        return Response('Improper data', status=406)


@app.route('/api/get_latest_data')
def get_latest_data():
    return jsonify(database.get_latest_data(666))

if __name__ == '__main__':
    if database.connected:
        log.info('Starting flask server')
        app.run(host='0.0.0.0', debug=True)
    else:
        log.info('Database not connected. Exiting App.')
