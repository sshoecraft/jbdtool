for r in pi win64 linux
do
	make TARGET=$r STATIC=yes clean
	make TARGET=$r STATIC=yes zip
	make TARGET=$r STATIC=yes clean
done
