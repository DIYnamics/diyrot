ROOT_DIR ?= $(CURDIR)
site ?=

default:

dev:
	rm -fdr $(ROOT_DIR)/$(site)/return $(ROOT_DIR)/$(site)/uploads
	mkdir -p -m 777 $(ROOT_DIR)/$(site)/return
	mkdir -p -m 777 $(ROOT_DIR)/$(site)/uploads
	make DEV=1 INCLUDES="/usr/local/Cellar/opencv/4.8.0_7/include/opencv4" -C dcppr clean
	make DEV=1 INCLUDES="/usr/local/Cellar/opencv/4.8.0_7/include/opencv4" -C dcppr install
	cd proc && python3 app.py
