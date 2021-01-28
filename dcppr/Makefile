.PHONY: derot radii prederot
default: radii derot

radii:
	g++ -O3 -I/usr/include/opencv4/ radii_check.cpp -o radii_check -L`pkg-config --libs opencv4`

derot:
	g++ -O3 -I/usr/include/opencv4/ derot.cpp -o derot -L`pkg-config --libs opencv4`

prederot:
	g++ -O3 -I/usr/include/opencv4/ prederot.cpp -o prederot -L`pkg-config --libs opencv4`

install-bin: radii derot prederot
	mkdir -p /var/www/$(site)/bin
	cp radii_check /var/www/$(site)/bin/radii_check
	cp derot /var/www/$(site)/bin/derot
	cp prederot /var/www/$(site)/bin/prederot
