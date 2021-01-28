install-nginx:
	cp $(site) /etc/nginx/sites-available/
	rm -f /etc/nginx/sites-enabled/default
	rm -f /etc/nginx/sites-enabled/$(site)
	ln -s /etc/nginx/sites-available/$(site) /etc/nginx/sites-enabled/$(site)
	systemctl restart nginx
