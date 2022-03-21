TARGET = ymirdb

CC = gcc

CFLAGS = -c -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security  -fsanitize=address
UFLAGS = -c -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security
SRC = ymirdb.c
OBJ = $(SRC:.c=.o)

# all:$(TARGET)

# $(TARGET):$(OBJ)
# 	$(CC) -o $@ $(OBJ)

# .SUFFIXES: .c .o

# .c.o:
# 	 $(CC) $(CFLAGS) $<
ymirdb: ymirdb.o 
	$(CC) ymirdb.o -o ymirdb -fsanitize=address

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
