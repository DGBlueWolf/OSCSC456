#-----------------------------------------------------------------------

# Use the GNU C/C++ compiler:
CC = gcc
CPP = g++

# COMPILER OPTIONS:

CFLAGS = -c

#OBJECT FILES
OBJS = run.o dsh.o

dsh: ${OBJS}
	${CC} -lm ${OBJS} -o dsh

dsh.o: dsh.c
	${CC} -c -lm dsh.c

run.o: run.c
	${CC} -c -lm run.c
#-----------------------------------------------------------------------
