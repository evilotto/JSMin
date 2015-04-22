CFLAGS=-O2
TCL_INCLUDE=${HOME}/devel/nsadmin/include
NS_INCLUDE=${HOME}/devel/nsadmin/include


jsmin.so: tcljsmin.o jsminlib.o
	gcc ${CFLAGS} --shared jsminlib.o tcljsmin.o -o jsmin.so -L${HOME}/devel/nsadmin/lib -ltcl8.6

nsjsmin.so: nstcljsmin.o jsminlib.o
	gcc ${CFLAGS} --shared jsminlib.o tcljsmin.o -o jsmin.so -L${HOME}/devel/nsadmin/lib -ltcl8.6 -lnsd

jsminlib.o: jsminlib.c
	gcc ${CFLAGS} -fPIC -c jsminlib.c  

tcljsmin.o: tcljsmin.c
	gcc ${CFLAGS} -I${TCL_INCLUDE} -fPIC -c tcljsmin.c  

nstcljsmin.o: tcljsmin.c
	gcc ${CFLAGS} -I${NS_INCLUDE} -DNAVISERVER -fPIC -c tcljsmin.c -o $@ 
