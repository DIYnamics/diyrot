# python server which calls c++ code in dcppr/
import os
import random
import string
import subprocess
from flask import Flask, request, send_file, send_from_directory, url_for, jsonify
from werkzeug.utils import secure_filename

# various settings meant for production environment
# note: DO NOT COMMIT CHANGES to this var. Instead, override (see below) during
# testing, if needed.
_root_dir = '/var/www/diyrot.epss.ucla.edu/'
# allow override by setting envvar (during testing)
if 'ROOT_DIR' in os.environ:
    _root_dir = os.environ['ROOT_DIR']
print('Current root dir: ' + _root_dir)
random.seed()

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = _root_dir + '/uploads'
# enable max 1G uploads. note that similar settings exist in nginx and uwsgi
app.config['MAX_CONTENT_LENGTH'] = 1000 * 1024 * 1024

# used in development only - equivalents exist for nginx in prod
# all are static file routes
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

@app.route('/spin-logo.png')
def spinlogo():
    return send_file('../static/spin-logo.png')

@app.route('/diynamics-logo.png')
def logo():
    return send_file('../static/diynamics-logo.png')

# actual production methods
@app.route('/upload/', methods=['POST'])
def save():
    '''
        save incoming video file under random name. call circle detection
        function to find a circle, and if found, generate a preview video with
        found parameters.
        if cannot be found, return a 500, with the random file name
        (a random name is desirable to prevent brute force downloading)
    '''
    try:
        # get form file object
        v = request.files['v']
        # pick name
        vname = ''.join(random.choices(string.ascii_letters, k=5)) + os.path.splitext(v.filename)[-1]
        # pick path
        vpath = os.path.join(app.config['UPLOAD_FOLDER'], vname)
        # save, close
        v.save(vpath)
        v.close()
        # detect circle and generate preview
        x,y,r = opencv_detect(vpath)
        preview_fn = opencv_preview(vname, x, y, r, request.form['rpm'])
    except Exception as e:
        # prints are connected to syslog and viewable thru monitoring
        print(" ".join(["error in upload", str(request.form), str(e)]), flush=True)
        return jsonify({'fn': vname}), 500
    return jsonify({'fn': vname, 'x': x, 'y': y, 'r': r, 'src': '/return/'+preview_fn}), 200

@app.route('/preview/', methods=['POST'])
def prev():
    try:
        # repreview a previously uploaded file
        preview_fn = opencv_preview(request.form['v'], \
             float(request.form['x']), \
             float(request.form['y']), \
             float(request.form['r']), \
             float(request.form['rpm']))
    except Exception as e:
        # prints are connected to syslog and viewable thru monitoring
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
        # derotated a video that passes preview
        r = opencv_derot(request.form['v'], \
             float(request.form['x']), \
             float(request.form['y']), \
             float(request.form['r']), \
             float(request.form['rpm']), \
             request.form['sbs'])
        return r, 200
    except Exception as e:
        # prints are connected to syslog and viewable thru monitoring
        print(" ".join(["error in derot", str(request.form), str(e)]), flush=True)
        return "", 500

@app.route('/count/')
def update_count():
    '''
    Site counter function. Dumbly read-increment-writes a file each time the
    endpoint is hit. Obviously loses count on concurrent access
    '''
    c = 0
    try:
        with open('count', 'r') as f:
            c = int(f.read())
    except:
        pass
    with open('count', 'w+') as f:
        f.write(str(c+1))
    return str(c), 200

def opencv_detect(vidfp):
    # calls radius checker with give video
    return subprocess.check_output([_root_dir + '/bin/radii_check', vidfp], text=True).split()

def opencv_preview(vidfn, x, y, r, rpm):
    # appends '-pre' before extension in filename; frontend attempts to load
    # this video on return
    fn, _ = os.path.splitext(vidfn)
    extn = '.mp4'
    fn += '-pre'
    # waits for return; assuming this is a quick job
    subprocess.check_call([_root_dir+'/bin/prederot',
                        os.path.join(_root_dir, 'uploads', vidfn),
                        str(x), str(y), str(r), str(rpm),
                        os.path.join(_root_dir, 'return', fn+extn)])
    return fn+extn

def opencv_derot(vidfn, x, y, r, rpm, sbs):
    # full quality derotation
    # start job and immediately return. up to frontend+nginx to keep track of
    # video derotation progress.
    fn, _ = os.path.splitext(vidfn)
    extn = '.mp4'
    # rename nicely with important metadata
    fn = "_".join(['diyrot', str(round(rpm))+'rpm', str(fn), 'derot'])
    if sbs == 'true':
        # run side by side job
        subprocess.Popen([_root_dir+'/bin/siderot',
                            os.path.join(_root_dir, 'uploads', vidfn),
                            str(x), str(y), str(r), str(rpm),
                            os.path.join(_root_dir, 'return', fn+extn)])
    else:
        # run regular job
        subprocess.Popen([_root_dir+'/bin/derot',
                            os.path.join(_root_dir, 'uploads', vidfn),
                            str(x), str(y), str(r), str(rpm),
                            os.path.join(_root_dir, 'return', fn+extn)])
    return fn+extn

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8081)
