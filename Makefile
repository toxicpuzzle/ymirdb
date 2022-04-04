TARGET = ymirdb

CC = gcc

CFLAGS = -c -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security  -fsanitize=address -fprofile-arcs -ftest-coverage
UFLAGS = -c -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security
SRC = ymirdb.c
OBJ = $(SRC:.c=.o)

ymirdb: ymirdb.o 
	$(CC) ymirdb.o -o ymirdb -fsanitize=address -fprofile-arcs -ftest-coverage

usanit:
	$(CC) $(UFLAGS) $(SRC)
	$(CC) ymirdb.o -o ymirdb

ymirdb.o: ymirdb.c
	$(CC) $(CFLAGS) $(SRC) 

test:
	bash test.sh

test_diff:
	bash test_diff.sh

clean:
	rm -f *.o *.obj $(TARGET)
