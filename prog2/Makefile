#-----------------------------------------------------------------------

# Use the GNU C/C++ compiler:
CC = gcc
CPP = g++

# COMPILER OPTIONS:

CFLAGS = -lm -pthread

#OBJECT FILES
OBJS = special.o run.o dsh.o

dsh: ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o dsh

dsh.o: dsh.c
	${CC} -c ${CFLAGS} dsh.c

run.o: run.c
	${CC} -c ${CFLAGS} run.c
	
special.o: special.c
	${CC} -c ${CFLAGS} special.c
#-----------------------------------------------------------------------
