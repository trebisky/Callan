# nothing to compile

all:	tos

get:
	cp ../First/first.img .

tos: ../toS/tos
	cp ../toS/tos .

install:	sloader tos
	cp sloader /usr/local/bin
	cp tos /usr/local/bin
