#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
# 

CC=gcc
LIB_DIR=../lib
INC_DIR=../include

all: exemplo

exemplo: t2fstst.c
	$(CC) -o main t2fstst.c -L$(LIB_DIR) -I$(INC_DIR) -lt2fs -Wall

clean:
	rm -rf *~ *.o main


