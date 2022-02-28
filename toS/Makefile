# nothing to compile

OBJS = tos.o srecord.o

tos:	$(OBJS)
	cc -o tos $(OBJS)

test:
	./tos first.img 0x10000
get:
	cp ../First/first.img .

# THE END
