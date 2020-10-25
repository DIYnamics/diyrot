import os
import random
import subprocess
from flask import Flask, request, send_file, send_from_directory, url_for
from werkzeug.utils import secure_filename

_root_dir = '/var/www/dpr-dev.epss.ucla.edu/'

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = _root_dir + '/uploads'
app.config['MAX_CONTENT_LENGTH'] = 100 * 1024 * 1024

# used in development only
@app.route('/')
def index():
    return send_file('index.html')

@app.route('/jquery.min.js')
def jquery():
    return send_file('jquery.min.js')

@app.route('/ndpr.js')
def netdprjs():
    return send_file('ndpr.js')

# actual production method
@app.route('/upload/', methods=['POST'])
def process():
    try:
        rpm, r, v = float(request.form['rpm']), float(request.form['r']), request.files['v']
        vpath = os.path.join(app.config['UPLOAD_FOLDER'], secure_filename(v.filename))
        v.save(vpath)
        v.close()
        x,y,r = opencv_detect(vpath, r)
        vout = opencv_derot(vpath, x, y, r, rpm)
    except e:
        print(e)
        return "bad_img", 400
    return url_for('sendback', fn=vout), 200

# only used to generate path
@app.route('/return/<fn>')
def sendback(fn):
    return send_from_directory(_root_dir + '/return', fn)

def opencv_detect(vidfp, r):
    return subprocess.check_output([_root_dir + '/bin/radii_check', vidfp, str(r)], text=True).split()

def opencv_derot(vidfp, x, y, r, rpm):
    return subprocess.check_output([_root_dir + '/bin/derot', vidfp, x, y, r, str(rpm), _root_dir + '/return/'], text=True)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
