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
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
EX_DIR=./exemplo

all: compile libmount exemplo

compile: $(SRC_DIR)/t2fs.c
	$(CC) -c $(SRC_DIR)/t2fs.c
	mv t2fs.o $(BIN_DIR)

libmount: $(BIN_DIR)/t2fs.o $(BIN_DIR)/apidisk.o
	ar crs libt2fs.a  $(BIN_DIR)/apidisk.o  $(BIN_DIR)/t2fs.o
	mv libt2fs.a $(LIB_DIR)

exemplo: $(EX_DIR)/t2fstst.c $(LIB_DIR)/libt2fs.a
	$(CC) -o main $(EX_DIR)/t2fstst.c -L$(LIB_DIR) -lt2fs -Wall

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~


