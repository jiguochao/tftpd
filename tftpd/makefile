CC = g++
OBJ = tftpd.o packetopt.o translate.o handlecmd.o
test : $(OBJ)
	$(CC) $^ -o $@ -g -Wall -pthread -std=c++11
%.o : %.cpp
	$(CC) -c  $^ -std=c++11
clean:
	rm *.o test
