


all: libsimplefs.a create_format app

libsimplefs.a: 	simplefs.c
	gcc -Wall -c simplefs.c
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a

create_format: create_format.c
	gcc -Wall -o create_format  create_format.c   -L. -lsimplefs

app: 	app.c
	gcc -Wall -o app app.c  -L. -lsimplefs

clean: 
	rm -fr *.o *.a *~ a.out app  vdisk create_format
