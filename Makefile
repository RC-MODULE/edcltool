edcltool: edcl.c edcl.h edcltool.c
	$(CC)  -o $(@) -rdynamic -Wl,-export-dynamic -lm -ldl $(^) /usr/lib/liblua.a 

install: edcltool	
	cp edcltool /usr/local/bin
clean:
	rm edcltool
