FLAGS=-g -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBCAP=1 -DUSE_LINUX_CAPABILITIES=1 -DHAVE_LIBPAM=1
all: invoked invoke grant test
SERVER_OBJ=common.o peer.o appdb.o auth.o book_inout.o
CLIENT_OBJ=common.o peer.o
GRANT_OBJ=common.o appdb.o peer.o 
LIBS=-lpam -lcap  -lpam libUseful-4/libUseful.a 

invoked: $(SERVER_OBJ) server.c libUseful-4/libUseful.a
	gcc $(FLAGS) -oinvoked $(SERVER_OBJ) server.c $(LIBS)

invoke: $(CLIENT_OBJ) client.c libUseful-4/libUseful.a 
	gcc $(FLAGS) -oinvoke $(CLIENT_OBJ)  client.c $(LIBS)

grant: $(GRANT_OBJ) grant.c libUseful-4/libUseful.a 
	gcc $(FLAGS) -ogrant $(GRANT_OBJ) grant.c $(LIBS)

common.o: common.h common.c
	gcc $(FLAGS) -c common.c

peer.o: peer.h peer.c
	gcc $(FLAGS) -c peer.c

appdb.o: appdb.h appdb.c
	gcc $(FLAGS) -c appdb.c

auth.o: auth.h auth.c
	gcc $(FLAGS) -c auth.c

book_inout.o: book_inout.h book_inout.c
	gcc $(FLAGS) -c book_inout.c

libUseful-4/libUseful.a:
	$(MAKE) -C libUseful-4

clean:
	rm -f *.o */*.o */*.so */*.a

test:
	echo "no tests"
