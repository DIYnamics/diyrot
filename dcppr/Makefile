# there are 2**3 binaries generated, for each combo of these bools:
# SIDE_BY_SIDE, PREVIEW, ADVANCED
# only the first macro is explicitly defined; the four combos of the latter
# two each have their own macro name

.PHONY: all install-bin clean Makefile derot.cpp radii_check.cpp

DEROT_BINARIES = derot prederot adv_prederot adv_derot sbs_derot sbs_prederot sbs_adv_derot sbs_adv_prederot
BINARIES = radii_check $(DEROT_BINARIES)

O = $(if DEV,-g,-O3)
INCLUDES = "/usr/local/Cellar/opencv/4.8.0_7/include/opencv4"

all: $(BINARIES)

radii_check: radii_check.cpp
	$(CXX) $(O) -I${INCLUDES} -std=c++17 radii_check.cpp `pkg-config --libs opencv4` -o $@

$(DEROT_BINARIES): derot.cpp
	$(CXX) $(if $(findstring sbs_, $@), -DSIDE_BY_SIDE) \
	-D$(if $(findstring adv_, $@),ADV_)$(if $(findstring pre, $@),PRE,DEROT) \
	$(O) -I${INCLUDES} -std=c++17 derot.cpp `pkg-config --libs opencv4` \
	-o $@

install-bin: all
	mkdir -p $(ROOT_DIR)/$(site)/bin
	for b in $(BINARIES); do cp $$b $(ROOT_DIR)/$(site)/bin/$$b; done

clean:
	rm -f $(BINARIES)
