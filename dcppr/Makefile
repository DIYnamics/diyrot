.PHONY: radii_check derot

default: radii derot

radii:
	g++ -I/usr/include/opencv4/ radii_check.cpp -o radii_check -L`pkg-config --libs opencv4`

derot:
	g++ -I/usr/include/opencv4/ derot.cpp -o derot -L`pkg-config --libs opencv4`

derot-test: derot radii-test
	./derot square.mp4 `./radii_check square.mp4 400` 10

radii-test: radii
	./radii_check square.mp4 400

install: radii derot
	cp radii_check /tmp/www/bin/radii_check
	cp derot /tmp/www/bin/derot
