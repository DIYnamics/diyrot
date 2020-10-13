SITE_NAME = dpr-dev.epss.ucla.edu

install-nginx:
	cp $(SITE_NAME) /etc/nginx/sites-available/
	rm -f /etc/nginx/sites-enabled/default
	rm -f /etc/nginx/sites-enabled/$(SITE_NAME)
	ln -s /etc/nginx/sites-available/$(SITE_NAME) /etc/nginx/sites-enabled/$(SITE_NAME)
	systemctl restart nginx
