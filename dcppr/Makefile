.PHONY: all install-bin clean

INCLUDES = "/usr/local/Cellar/opencv/4.8.0_7/include/opencv4"
BINARIES := radii derot prederot adv_prederot adv_derot sbs_derot sbs_prederot sbs_adv_derot sbs_adv_prederot

all: $(BINARIES)

radii: radii_check.cpp
	$(CXX) -O3 -I${INCLUDES} -std=c++17 radii_check.cpp `pkg-config --libs opencv4` -o $@

%::derot.cpp
	$(CXX) $(if $(findstring sbs_, $@), -DSIDE_BY_SIDE) \
	-D$(if $(findstring adv_, $@),ADV_)$(if $(findstring pre, $@),PRE,DEROT) \
	-O3 -I${INCLUDES} -std=c++17 derot.cpp `pkg-config --libs opencv4` \
	-o $@

install-bin: all
	mkdir -p $(ROOT_DIR)/$(site)/bin
	for b in $(BINARIES); do cp $$b $(ROOT_DIR)/$(site)/bin/$$b; done

clean:
	rm -f $(BINARIES)
