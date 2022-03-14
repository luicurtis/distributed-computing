EXECS=send_recv ring
MPICC?=mpicc

all: ${EXECS}

send_recv: send_recv.c
	${MPICC} -o send_recv send_recv.c

ring: ring.c
	${MPICC} -o ring ring.c

clean:
	rm -f ${EXECS}