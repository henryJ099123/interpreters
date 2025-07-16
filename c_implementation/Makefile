CC= clang
CFLAGS= -Wall
EX= cloxExec
SOURCES=$(wildcard *.c)
OBJS= $(SOURCES:.c=.o)
COMMON= common.h

all: $(EX)

clean:
	rm $(EX)
	rm *.o

$(EX): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# anything that starts with .o: compile it first
%.o: %.c %.h $(COMMON)
	$(CC) $(CFLAGS) -c $< -o $@

# if the .h files change, change the respective ones
# %.c: %.h
