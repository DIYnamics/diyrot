install-proc: install-static install-uwsgi
install-static:
	cp index.html ndpr.js jquery.min.js /tmp/www/

install-uwsgi:
	sed -i 's|User=$$|User='$(shell whoami)'|' netdpr.service
	sed -i 's|WorkingDirectory=$$|WorkingDirectory='$(shell pwd)'|' netdpr.service
	

