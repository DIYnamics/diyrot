.PHONY: all radii derot prederot adv_prederot adv_derot sbs_derot sbs_prederot sbs_adv_derot sbs_adv_prederot

INCLUDES = "/usr/local/Cellar/opencv/4.8.0_7/include/opencv4"
default: all

all: radii derot prederot adv_prederot adv_derot sbs_derot sbs_prederot sbs_adv_derot sbs_adv_prederot
radii:
	g++ -O3 -I${INCLUDES} -std=c++17 radii_check.cpp -o $@ `pkg-config --libs opencv4`

derot:
	g++ -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

prederot:
	g++ -DPRE -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

adv_derot:
	g++ -DADV_DEROT -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

adv_prederot:
	g++ -DADV_PRE -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

sbs_derot:
	g++ -O3 -DSIDE_BY_SIDE -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

sbs_prederot:
	g++ -DPRE -DSIDE_BY_SIDE -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

sbs_adv_derot:
	g++ -DADV_DEROT -DSIDE_BY_SIDE -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

sbs_adv_prederot:
	g++ -DADV_PRE -DSIDE_BY_SIDE -O3 -I${INCLUDES} -std=c++17 new_derot.cpp -o $@ `pkg-config --libs opencv4`

install-bin: radii derot prederot
	mkdir -p $(ROOT_DIR)/$(site)/bin
	cp radii_check $(ROOT_DIR)/$(site)/bin/radii_check
	cp derot $(ROOT_DIR)/$(site)/bin/derot
	cp prederot $(ROOT_DIR)/$(site)/bin/prederot
	cp adv_prederot $(ROOT_DIR)/$(site)/bin/adv_prederot

test: adv_derot
	rm -f out.mp4
	( date && time ./adv_derot ../example_irl.mp4 462 260 250 13.3 1 out.mp4 1 0,170 0 0) 2>&1 | tee -a runtimes

