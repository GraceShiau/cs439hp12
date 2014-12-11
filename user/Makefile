PROGS = shell ls shutdown echo cat TestProgram login TestGame .grace ping np TestServer

all : $(PROGS)

-include ../common.mak

shell : CCFILES=
shell : CFILES=shell.c libc.c heap.c

echo : CCFILES=
echo : CFILES=echo.c libc.c heap.c

ls : CCFILES=
ls : CFILES=ls.c libc.c heap.c

shutdown : CCFILES=
shutdown : CFILES=shutdown.c libc.c heap.c

cat : CCFILES=
cat : CFILES=cat.c libc.c heap.c

login : CCFILES=login.cc md5.cc
login : CFILES=libc.c heap.c

md5 : CCFILES=md5.cc
md5 : CFILES=libc.c heap.c

.grace : CCFILES=grace.cc
.grace : CFILES=libc.c heap.c

TestProgram : CFILES=libc.c heap.c
TestProgram : CCFILES=TestProgram.cc DesktopWindowManager.cc

TestGame : CFILES=libc.c heap.c
TestGame : CCFILES=TestGame.cc

ping : CFILES=libc.c heap.c
ping : CCFILES=Ping.cc

np : CFILES=libc.c heap.c
np : CCFILES=NetworkPixel.cc

TestServer : CFILES=libc.c heap.c
TestServer : CCFILES=TestServer.cc

$(PROGS) : % : Makefile $(OFILES)
	ld -N -m elf_i386 -e start -Ttext=0x80000000 -o $@ $(OFILES)

clean ::
	rm -f $(PROGS)
