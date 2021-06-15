BNAME=balloon
BFILE=Balloon.c

CC=gcc
CFLAGS=-I.

build: .balloon
	
all: .balloon 
	
clean:
	${RM} ${BNAME}

.balloon: Balloon.c
	$(CC) -O2 -fopenmp -o ${BNAME} ${BFILE}

