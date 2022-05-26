sigrok2ega: sigrok2ega.c
	$(CC) -O2 -W -Wall sigrok2ega.c -o sigrok2ega -lSDL2

unpack:
	7z e data\

test: sigrok2ega
	cmd.exe /c "type VEGA.bin | .\sigrok2ega.exe"
	
test2: sigrok2ega
	cmd.exe /c "type VEGA-2.bin | .\sigrok2ega.exe"

clean:
	$(RM) sigrok2ega.exe
