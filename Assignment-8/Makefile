all: proc_2.c mmu.c master.c sched2.c
	gcc master.c -o master -lm
	gcc sched2.c -o scheduler -lm
	gcc mmu.c -o mmu -lm
	gcc proc_2.c -o process -lm

clean:
	rm process master scheduler mmu