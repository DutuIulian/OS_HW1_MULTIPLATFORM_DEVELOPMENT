CC=cl
CFLAGS=/MD

all: so-cpp

build: so-cpp

so-cpp: so-cpp.obj table.obj
    $(CC) /EHsc /Fe so-cpp so-cpp.obj table.obj $(CFLAGS)

so-cpp.obj: so-cpp.c
	$(CC) /c so-cpp.c $(CFLAGS)

table.obj: table.c
	$(CC) /c table.c $(CFLAGS)
 
.PHONY: clean
clean:
	del so-cpp.exe *.obj