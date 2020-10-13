.PHONY: install
install:
	rm -fdr /tmp/www/
	mkdir -p /tmp/www/return
	mkdir -p /tmp/www/uploads
	make -C install/ install-nginx
	make -C dcppr/ install-bin
	make -C proc/ install-proc
