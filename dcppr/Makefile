.PHONY: derot radii prederot siderot all

INCLUDES = "/usr/local/Cellar/opencv/4.7.0/include/opencv4"
default: install-bin

all: radii derot siderot prederot

radii:
	g++ -O3 -I${INCLUDES} -std=c++11 radii_check.cpp -o radii_check `pkg-config --libs opencv4`

derot:
	g++ -O3 -I${INCLUDES} -std=c++11 derot.cpp -o derot `pkg-config --libs opencv4`

siderot:
	g++ -O3 -I${INCLUDES} -std=c++11 siderot.cpp -o siderot `pkg-config --libs opencv4`

prederot:
	g++ -O3 -I${INCLUDES} -std=c++11 prederot.cpp -o prederot `pkg-config --libs opencv4`

install-bin: radii derot prederot siderot
	mkdir -p $(ROOT_DIR)/$(site)/bin
	cp radii_check $(ROOT_DIR)/$(site)/bin/radii_check
	cp derot $(ROOT_DIR)/$(site)/bin/derot
	cp siderot $(ROOT_DIR)/$(site)/bin/siderot
	cp prederot $(ROOT_DIR)/$(site)/bin/prederot
