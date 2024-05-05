ROOT_DIR ?= $(CURDIR)
site ?=

default:

dev:
	rm -fdr $(ROOT_DIR)/$(site)/return $(ROOT_DIR)/$(site)/uploads
	mkdir -p -m 777 $(ROOT_DIR)/$(site)/return
	mkdir -p -m 777 $(ROOT_DIR)/$(site)/uploads
	make -C dcppr clean
	make DEV=1 INCLUDES="/usr/local/Cellar/opencv/4.9.0_7/include/opencv4" -C dcppr -j$(sysctl -n hw.ncpu) install
	cd proc && ROOT_DIR=$(CURDIR) python3 app.py
