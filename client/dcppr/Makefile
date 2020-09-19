.PHONY: radii_check derot

default: radii derot

radii:
	g++ -I/usr/include/opencv4/ radii_check.cpp -o radii_check -L`pkg-config --libs opencv4`

derot:
	g++ -I/usr/include/opencv4/ derot.cpp -o derot -L`pkg-config --libs opencv4`

derot-test: derot radii-test
	./derot IMG_0367.mp4 `./radii_check IMG_0367.mp4 400` 10

radii-test: radii
	./radii_check IMG_0367.mp4 400
