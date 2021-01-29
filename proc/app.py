import os
import random
import string
import subprocess
from flask import Flask, request, send_file, send_from_directory, url_for, jsonify
from werkzeug.utils import secure_filename

_root_dir = '/var/www/dpr-dev.epss.ucla.edu/'
if 'ROOT_DIR' in os.environ:
    _root_dir = os.environ['ROOT_DIR']
print('Current root dir: ' + _root_dir)
random.seed()

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = _root_dir + '/uploads'
app.config['MAX_CONTENT_LENGTH'] = 1000 * 1024 * 1024

# used in development only
@app.route('/')
def index():
    return send_file('../static/index.html')

@app.route('/favicon.ico')
def bruh():
    return send_file('../static/favicon.ico')

@app.route('/ndpr.js')
def netdprjs():
    return send_file('../static/ndpr.js')

@app.route('/return/<vid>')
def retvid(vid):
    return send_file(_root_dir + '/return/' + vid)

# actual production methods
@app.route('/upload/', methods=['POST'])
def save():
    try:
        v = request.files['v']
        vname = ''.join(random.choices(string.ascii_letters, k=20)) + os.path.splitext(v.filename)[-1]
        vpath = os.path.join(app.config['UPLOAD_FOLDER'], vname)
        v.save(vpath)
        v.close()
        x,y,r = opencv_detect(vpath)
        preview_fn = opencv_preview(vname, x, y, r, request.form['rpm'])
    except Exception as e:
        print(" ".join(["error in upload", str(request.form), str(e)]), flush=True)
        return jsonify({'fn': vname}), 400
    return jsonify({'fn': vname, 'x': x, 'y': y, 'r': r, 'src': '/return/'+preview_fn}), 200

@app.route('/preview/', methods=['POST'])
def prev():
    try:
        preview_fn = opencv_preview(request.form['v'], \
             float(request.form['x']), \
             float(request.form['y']), \
             float(request.form['r']), \
             float(request.form['rpm']))
    except Exception as e:
        print(" ".join(["error in preview", str(request.form), str(e)]), flush=True)
        return "", 500
    return jsonify({'fn': request.form['v'],
                    'x': request.form['x'],
                    'y': request.form['y'],
                    'r': request.form['r'],
                    'src': '/return/'+preview_fn}), 200

@app.route('/derot/', methods=['POST'])
def derot():
    try:
        r = opencv_derot(request.form['v'], \
             float(request.form['x']), \
             float(request.form['y']), \
             float(request.form['r']), \
             float(request.form['rpm']))
        return r, 200
    except Exception as e:
        print(" ".join(["error in derot", str(request.form), str(e)]), flush=True)
        return "", 500


def opencv_detect(vidfp):
    return subprocess.check_output([_root_dir + '/bin/radii_check', vidfp], text=True).split()

def opencv_preview(vidfn, x, y, r, rpm):
    fn, extn = os.path.splitext(vidfn)
    fn += '-pre'
    subprocess.check_call([_root_dir+'/bin/prederot',
                        os.path.join(_root_dir, 'uploads', vidfn),
                        str(x), str(y), str(r), str(rpm),
                        os.path.join(_root_dir, 'return', fn+extn)])
    return fn+extn

def opencv_derot(vidfn, x, y, r, rpm):
    fn, extn = os.path.splitext(vidfn)
    fn = "_".join(['diyrot', str(round(rpm))+'rpm', str(fn), 'derot'])
    subprocess.Popen([_root_dir+'/bin/derot',
                        os.path.join(_root_dir, 'uploads', vidfn),
                        str(x), str(y), str(r), str(rpm),
                        os.path.join(_root_dir, 'return', fn+extn)])
    return fn+extn

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
