FLAGS=-g -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64 -DHAVE_LIBUSEFUL_4=1 -DHAVE_LIBSSL=1 -DHAVE_LIBCRYPTO=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBCAP=1 -DUSE_LINUX_CAPABILITIES=1 -DHAVE_LIBPAM=1
all: invoked invoke grant test
SERVER_OBJ=common.o peer.o appdb.o auth.o book_inout.o
CLIENT_OBJ=common.o peer.o
GRANT_OBJ=common.o appdb.o peer.o 
LIBS=-lpam -lcap -lcrypto -lssl -lUseful-4  -lpam  

invoked: $(SERVER_OBJ) server.c 
	$(CC) $(FLAGS) -oinvoked $(SERVER_OBJ) server.c $(LIBS)

invoke: $(CLIENT_OBJ) client.c  
	$(CC) $(FLAGS) -oinvoke $(CLIENT_OBJ)  client.c $(LIBS)

grant: $(GRANT_OBJ) grant.c  
	$(CC) $(FLAGS) -ogrant $(GRANT_OBJ) grant.c $(LIBS)

common.o: common.h common.c
	$(CC) $(FLAGS) -c common.c

peer.o: peer.h peer.c
	$(CC) $(FLAGS) -c peer.c

appdb.o: appdb.h appdb.c
	$(CC) $(FLAGS) -c appdb.c

auth.o: auth.h auth.c
	$(CC) $(FLAGS) -c auth.c

book_inout.o: book_inout.h book_inout.c
	$(CC) $(FLAGS) -c book_inout.c

libUseful-4/libUseful.a:
	$(MAKE) -C libUseful-4

clean:
	rm -f *.o */*.o */*.so */*.a invoked invoke grant

test:
	echo "no tests"
