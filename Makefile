site = diyrot.epss.ucla.edu
#site = dpr-dev.epss.ucla.edu
export site

default:
	systemctl status netdpr.service
	systemctl status nginx.service

nginx-log:
	tail /var/log/nginx/error.log -n 20

uwsgi-log:
	journalctl _SYSTEMD_UNIT=netdpr.service | tail -n 20

install:
	mkdir -p /var/www/$(site)/return -m 777
	mkdir -p /var/www/$(site)/uploads -m 777
	make -C static/ install-static
	make -C dcppr/ install-bin
	make -C proc/ install-proc
	make -C nginx/ install-nginx

uninstall:
	systemctl stop nginx.service
	systemctl stop netdpr.service
	rm -fdr /var/www/$(site)/

reinstall: uninstall install
