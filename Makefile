


all: libsimplefs.a create_format app

libsimplefs.a: 	simplefs.c
	gcc -Wall -c -g simplefs.c
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a

create_format: create_format.c
	gcc -g -Wall -o create_format  create_format.c   -L. -lsimplefs

app: 	app.c
	gcc -g -Wall -o app app.c  -L. -lsimplefs

clean: 
	rm -fr *.o *.a *~ a.out app  vdisk create_format
