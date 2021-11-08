CC=gcc
LIBS=-lpthread

all: naive_counter naive_counter_plus atomic_counter scalable_counter

naive_counter:
	$(CC) -o naive_counter naive_counter.c $(LIBS)	

naive_counter_plus:
	$(CC) -o naive_counter_plus naive_counter_plus.c $(LIBS)	

atomic_counter:
	$(CC) -o atomic_counter atomic_counter.c $(LIBS)	

scalable_counter:
	$(CC) -o scalable_counter scalable_counter.c $(LIBS)	

clean:
	rm naive_counter naive_counter_plus atomic_counter scalable_counter
