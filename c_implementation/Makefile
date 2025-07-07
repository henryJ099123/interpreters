CC= clang
CFLAGS= -Wall
EX= cloxExec
SOURCES=$(wildcard *.c)
OBJS= $(SOURCES:.c=.o)

all: $(EX)

clean:
	rm $(EX)
	rm *.o

$(EX): $(OBJS)
	$(CC) $^ -o $@

# anything that starts with .o: compile it first
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

