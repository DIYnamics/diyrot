.PHONY: radii_check derot

default: radii derot

radii:
	g++ -I/usr/include/opencv4/ radii_check.cpp -o radii_check -L`pkg-config --libs opencv4`

derot:
	g++ -I/usr/include/opencv4/ derot.cpp -o derot -L`pkg-config --libs opencv4`

install-bin: radii derot
	mkdir -p /tmp/www/bin/
	cp radii_check /tmp/www/bin/radii_check
	cp derot /tmp/www/bin/derot
