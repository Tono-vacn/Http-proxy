from datetime import datetime, timedelta
import json
import pytz
from flask import Flask, make_response

app = Flask(__name__)

@app.route('/')
def test_response():
    gmt_tz = pytz.timezone('GMT')
    current_time = datetime.now(gmt_tz)
    expire_time = current_time + timedelta(seconds=60)
    
    response = make_response(json.dumps({'message': 'Hello, World!'}))
    response.headers['Content-Type'] = 'application/json'
    response.headers['Cache-Control'] = 'max-age=15, must-revalidate'
    # response.headers['Last-Modified'] = current_time.strftime('%a, %d %b %Y %H:%M:%S %Z')
    response.headers['Expires'] = expire_time.strftime('%a, %d %b %Y %H:%M:%S %Z')
    # response.headers["ETag"] = "3e6-160e3f3a5b8"
    
    
    return response
  
if __name__ == '__main__':
    app.run(debug=True, host = '0.0.0.0')