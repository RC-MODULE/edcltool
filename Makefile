edcltool: edcl.c edcl.h edcltool.c
	$(CC) $(^) -o $(@) -llua

install: edcltool	
	cp edcltool /usr/local/bin
