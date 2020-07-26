FLAGS=-g -DHAVE_LIBPAM=1
all: invoked invoke grant test
SERVER_OBJ=common.o peer.o appdb.o auth.o book_inout.o
CLIENT_OBJ=common.o peer.o
GRANT_OBJ=common.o appdb.o peer.o
LIBS= -lcrypto -lssl -lUseful-4  -lcap -lpam

invoked: $(SERVER_OBJ) server.c 
	gcc $(FLAGS) -oinvoked $(SERVER_OBJ) server.c $(LIBS)

invoke: $(CLIENT_OBJ) client.c
	gcc $(FLAGS) -oinvoke $(CLIENT_OBJ)  client.c $(LIBS)

grant: $(GRANT_OBJ) grant.c
	gcc $(FLAGS) -ogrant $(GRANT_OBJ) grant.c $(LIBS)

test: $(CLIENT_OBJ) test.c
	gcc $(FLAGS) -otest $(CLIENT_OBJ) test.c $(LIBS)

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


clean:
	rm -f *.o
