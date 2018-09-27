all:  edt

edt:  edt.c scz_decompress.c  scz_routines.c
	cc -w -O edt.c -o edt

clean:
	rm -f edt
