all: process.c queue.c sched.c queue.h gen.c prio_queue.h prio_queue.c elem.h
	gcc process.c -o process
	gcc gen.c -o gen
	gcc queue.c -c
	gcc sched.c -c
	gcc prio_queue.c -c
	gcc queue.o sched.o prio_queue.o -o sched