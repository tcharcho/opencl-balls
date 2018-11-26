a4: a4.c
	gcc a4.c -o a4 -lncurses -lm -framework opencl

clean:
	rm -f phys a4


