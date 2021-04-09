.PHONY: derot radii prederot siderot
default: install-bin

radii:
	g++ -O3 -I/usr/include/opencv4/ radii_check.cpp -o radii_check -L`pkg-config --libs opencv4`

derot:
	g++ -O3 -I/usr/include/opencv4/ derot.cpp -o derot -L`pkg-config --libs opencv4`

siderot:
	g++ -O3 -I/usr/include/opencv4/ siderot.cpp -o siderot -L`pkg-config --libs opencv4`

prederot:
	g++ -O3 -I/usr/include/opencv4/ prederot.cpp -o prederot -L`pkg-config --libs opencv4`

install-bin: radii derot prederot siderot
	mkdir -p $(ROOT_DIR)/$(site)/bin
	cp radii_check $(ROOT_DIR)/$(site)/bin/radii_check
	cp derot $(ROOT_DIR)/$(site)/bin/derot
	cp siderot $(ROOT_DIR)/$(site)/bin/siderot
	cp prederot $(ROOT_DIR)/$(site)/bin/prederot
