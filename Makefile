SRC=./src
INC=./
OBJ=./obj

CC=gcc
CFLAGS=-Wall -pedantic -ansi -g -I $(INC)
LIBRARIES=-lrt -pthread

ARG_FILE=./Data/DataMedium.dat
ARG_N_LEVELS=5
ARG_N_PROCESSES=10
ARG_DELAY=5

.PHONY: clean_objects clean_program clean_doc clean run runv doc

##############################################

all: sort

sort: $(OBJ)/main.o $(OBJ)/sort.o $(OBJ)/utils.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBRARIES)



##############################################

$(OBJ)/main.o: main.c sort.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/sort.o: sort.c sort.h global.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/utils.o: utils.c utils.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/solution.o: main.c sort.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/workers.o: main.c sort.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

##############################################

clean_objects:
	@echo "Cleaning objects..."
	@rm -f $(OBJ)/*.o

clean_program:
	@echo "Cleaning program..."
	@rm -f sort solution

clean: clean_objects clean_program

##############################################

run: sort
	@echo "Launching program..."
	@sleep 1
	@./sort $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY)

runv: sort
	@echo "Launching program with valgrind..."
	@sleep 1
	@valgrind ./sort $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY)

runve: sort
	@echo "Launching program with valgrind and checking errors..."
	@sleep 1
	@valgrind ./sort $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) \
	$(ARG_DELAY) 2>&1 | grep 'error\|alloc'

runvfull: sort
	@echo "Launching program with valgrind..."
	@sleep 1
	@valgrind --track-origins=yes --leak-check=full ./sort $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY) 



run_small: sort
	@./sort ./Data/DataSmall.dat 5 10 100

run_medium: sort
	@./sort ./Data/DataMedium.dat 10 15 15

run_large: sort
	@./sort ./Data/DataLarge.dat 10 200 1
