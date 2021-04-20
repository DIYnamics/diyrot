# DIYrotate
No setup version of DigiPyRo. Use from your browser!

## Software Architecture

Here is a sorted list of programs DIYrotate uses to process video, in order of
increasing time:
- nginx/uwsgi configs in proc (see Ansible playbook for production scripts)
- static/index.html (render webpage)
- static/ndpr.js (control webpage, submission to server)
- proc/app.py (receive requests, dispatch to C++ code)
- dcppr/radii_check.cpp (find rotation circle)
- dcppr/prederot.cpp (generate preview)
- static/ndpr.js (check preview, fix, resubmit / submit for full derot)
- either dcppr/derot.cpp or static/siderot.cpp (full derot)

## Server requirements

To compile DIYrotate components, one needs the following debian packages (or
equivalents for other distros):

```
python3
ffmpeg
libavcodec-dev
libavformat-dev
libavutil-dev
libswscale-dev
libopencv-core-dev
libopencv-imgproc-dev
libopencv-imgcodecs-dev
libopencv-videoio-dev
uwsgi
uwsgi-plugin-python3
```

In addition, one needs the following python packages:

```
flask
```

## Server info

*Please note the nginx/uwsgi targets have been superseded by an Ansible playbook
located
[here](https://github.com/ucla-earth-planetary-and-space-sciences/diyrotate-playbooks/tree/main/roles/dpr-production-deploy).*  
*The targets are still provided for reference.*

Running `make install` will create the following folders and files:

```
/bin/derot
/bin/radii-check
/var/www/diyrot.epss.ucla.edu/*
/etc/nginx/{sites-available,sites-enabled}/diyrot.epss.ucla.edu
/etc/systemd/system/netdpr.service
```

A systemd service will be created to run uwsgi under the installing user
(ignoring sudo), and both that service and nginx will be started.

In addition, the default nginx site will be deleted.
