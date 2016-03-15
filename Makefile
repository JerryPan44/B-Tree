CC	= gcc
SOURCES = AM.c
FLAGS   = -g
all: exe1 exe2 exe3 exe4 gerTest1 gerTest2 gerTest2 gerTest3
	 
exe1: $(SOURCES) main1.c 
	$(CC) $(FLAGS) $(SOURCES) main1.c -o exe1 BF/BF_32.a

exe2: $(SOURCES) main2.c 
	$(CC) $(FLAGS) $(SOURCES) main2.c -o exe2 BF/BF_32.a

exe3: $(SOURCES) main3.c 
	$(CC) $(FLAGS) $(SOURCES) main3.c -o exe3 BF/BF_32.a

exe4: $(SOURCES) main4.c 
	$(CC) $(FLAGS) $(SOURCES) main4.c -o exe4 BF/BF_32.a
	
gerTest1: $(SOURCES) myTestMain.c 
	$(CC) -O3 $(SOURCES) myTestMain.c -o gerTest1 BF/BF_32.a

gerTest2: $(SOURCES) gerasimosTestMain2.c 
	$(CC) -O3 $(SOURCES) gerasimosTestMain2.c -o gerTest2 BF/BF_32.a	
	
gerTest3: $(SOURCES) gerasimosTestMain2.c 
	$(CC) -g3 $(SOURCES) gerasimosTestMain3.c -o gerTest3 BF/BF_32.a	
	
exeAll: ./exe1 ./exe2 ./exe3 ./exe4
clean:
	rm exe1 exe2 exe3 exe4 gerTest1 gerTest2 gerTest3
