install-proc:
	sed -i 's|User=$$|User='$(shell logname)'|' netdpr.service
	sed -i 's|WorkingDirectory=$$|WorkingDirectory='$(shell pwd)'|' netdpr.service
	cp netdpr.service /etc/systemd/system/netdpr.service
	systemctl daemon-reload
	systemctl start netdpr.service
	

