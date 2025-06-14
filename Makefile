ROOT_DIR ?= $(CURDIR)
site ?=

default:

dev:
	rm -fr $(ROOT_DIR)/$(site)/return $(ROOT_DIR)/$(site)/uploads
	mkdir -p -m 777 $(ROOT_DIR)/$(site)/return
	mkdir -p -m 777 $(ROOT_DIR)/$(site)/uploads
	make -C dcppr clean
	make DEV=1 -C dcppr -j$(nproc) install
	cd proc && ROOT_DIR=$(CURDIR) python3 app.py

osx:
	make DEV=1 INCLUDES="/usr/local/Cellar/opencv/4.10.0_12/include/opencv4" -C dcppr -j$(sysctl -n hw.ncpu)

test:
	./dcppr/adv_derot ./example_irl.mp4 462 260 252 13.3 out.mp4 1 0,250 1 0
