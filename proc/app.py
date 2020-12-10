import os
import random
import string
import subprocess
from flask import Flask, request, send_file, send_from_directory, url_for, jsonify
from werkzeug.utils import secure_filename

_root_dir = '/var/www/diyrot.epss.ucla.edu/'
random.seed()

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = _root_dir + '/uploads'
app.config['MAX_CONTENT_LENGTH'] = 1000 * 1024 * 1024

# used in development only
@app.route('/')
def index():
    return send_file('../static/index.html')

@app.route('/jquery.min.js')
def jquery():
    return send_file('../static/jquery.min.js')

@app.route('/favicon.ico')
def bruh():
    return send_file('../static/favicon.ico')

@app.route('/ndpr.js')
def netdprjs():
    return send_file('../static/ndpr.js')

# actual production method
@app.route('/upload/', methods=['POST'])
def save():
    try:
        r, v = float(request.form['r']), request.files['v']
        vname = ''.join(random.choices(string.ascii_letters, k=20)) + os.path.splitext(v.filename)[-1]
        vpath = os.path.join(app.config['UPLOAD_FOLDER'], vname)
        v.save(vpath)
        v.close()
        x,y,r = opencv_detect(vpath, r)
    except subprocess.CalledProcessError:
        print('errored')
        return jsonify({'fn': vname}), 400
    except:
        return {}, 500
    print('succ')
    return jsonify({'fn': vname, 'x': x, 'y': y, 'r': r}), 200

@app.route('/detect/', methods=['POST'])
def redetect():
    # check existance of file, and try to redetect based on the parameters given
    pass

@app.route('/derot/', methods=['POST'])
def derot():
    try:
        r = opencv_derot(request.form['v'], \
             float(request.form['x']), \
             float(request.form['y']), \
             float(request.form['r']), \
             float(request.form['rpm']))
        return r, 200
    except:
        return "", 500


def opencv_detect(vidfp, r):
    return subprocess.check_output([_root_dir + '/bin/radii_check', vidfp, str(r)], text=True).split()

def opencv_derot(vidfn, x, y, r, rpm):
    fn, extn = os.path.splitext(vidfn)
    fn += "-derot"
    subprocess.Popen([_root_dir+'/bin/derot',
                        os.path.join(_root_dir, 'uploads', vidfn),
                        str(x), str(y), str(r), str(rpm),
                        os.path.join(_root_dir, 'return', fn+extn)])
    return fn+extn

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
