default:
	systemctl status netdpr.service
	systemctl status nginx.service

.PHONY: install
install:
	rm -fdr /tmp/www/
	mkdir -p /tmp/www/return
	mkdir -p /tmp/www/uploads
	make -C static/ install-static
	make -C dcppr/ install-bin
	make -C proc/ install-proc
	make -C nginx/ install-nginx

