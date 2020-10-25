default:
	systemctl status netdpr.service
	systemctl status nginx.service

nginx-log:
	tail /var/log/nginx/error.log -n 20

uwsgi-log:
	journalctl _SYSTEMD_UNIT=netdpr.service | tail -n 20

install:
	systemctl stop nginx.service
	systemctl stop netdpr.service
	rm -fdr /var/www/dpr-dev.epss.ucla.edu/
	mkdir -p /var/www/dpr-dev.epss.ucla.edu/return -m 777
	mkdir -p /var/www/dpr-dev.epss.ucla.edu/uploads -m 777
	make -C static/ install-static
	make -C dcppr/ install-bin
	make -C proc/ install-proc
	make -C nginx/ install-nginx

