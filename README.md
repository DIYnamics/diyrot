# DIYrotate

No setup version of DigiPyRo. Use from your browser!

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

Running `make install` will create the following folders and files:

```
/bin/derot
/bin/radii-check
/var/www/diyrot.epss.ucla.edu/*
/etc/nginx/{sites-available, sites-enabled}/diyrot.epss.ucla.edu
/etc/systemd/system/netdpr.service
```

A systemd service will be created to run uwsgi under the installing user
(ignoring sudo), and both that service and nginx will be started.

In addition, the default nginx site will be deleted.
