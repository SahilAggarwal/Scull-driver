# Scull-driver
Very simple implementation of scull driver.

Reference LDD3. 
Chapter 3: Character Driver

Usage:  

	o $ make
	
	o $ insmod scull.ko
	
	o $ cat /proc/devinfo

	o Note the major number corresponding to "scull"

	o $ mknod /dev/scull0 -c <major number> <minor number>
	  This will create character file /dev/scull0
	
	o Info of device can be seen in /proc/scullseq


Very detailed documentation of code coming soon.
