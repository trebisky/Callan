# nothing to compile

all:	tos

get:
	cp ../First/first.img .

tos: ../toS/tos
	cp ../toS/tos .

# the reverse of install,
# because I edit the file in /usr/local/bin
# then want to copy it back here to commit
suck:
	cp /usr/local/bin/sloader .

install:	sloader tos
	cp sloader /usr/local/bin
	cp tos /usr/local/bin
