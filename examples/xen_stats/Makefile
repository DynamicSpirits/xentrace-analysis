CC=clang -g -Wall -m32
INC=-I../../include

OBJS=	Event.o\
	EventHandler.o\
	Reader.o\
	CpuList.o\
	Common.o\
	Num_Events.o\
	Num_Exceptions.o\
	Num_Exceptions_in_xen.o\
	Num_Interrupts.o\
	Num_Interrupts_in_xen.o\
	Num_Hypercalls.o\
	Lost_records.o

vpath %.h ../../include
vpath %.h .
vpath %.c ../../src
vpath %.c .

all: main.c $(OBJS) Macros.h Externals.h
	$(CC) $(INC) $<  *.o -o ../../bin/Xen_stats.out
	rm -f *.o

Event.o: Event.c Event.h Macros.h
	$(CC) $(INC) -c $< -o $@

EventHandler.o: EventHandler.c EventHandler.h Macros.h list.h
	$(CC) $(INC) -c $< -o $@

Reader.o: Reader.c Reader.h Macros.h
	$(CC) $(INC) -c $< -o $@

CpuList.o: CpuList.c CpuList.h list.h Event.h
	$(CC) $(INC) -c $< -o $@

Common.o: Common.c Common.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Num_Events.o: Num_Events.c Common.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Num_Exceptions.o: Num_Exceptions.c Common.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Num_Exceptions_in_xen.o: Num_Exceptions_in_xen.c Common.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Num_Interrupts.o: Num_Interrupts.c Common.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Num_Interrupts_in_xen.o: Num_Interrupts_in_xen.c Num_Interrupts_in_xen.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Num_Hypercalls.o: Num_Hypercalls.c Num_Hypercalls.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

Lost_records.o: Lost_records.c Lost_records.h Macros.h Externals.h
	$(CC) $(INC) -c $< -o $@

clean: 
	rm -rf ../../bin/Xen_stats.out
	rm -rf *.o
