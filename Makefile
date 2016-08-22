mie_string_test.exe: mie_string_test.cpp mie_string_x64.o mie_string.h
	$(CXX) -Ofast -march=native -Wall -Wextra mie_string_test.cpp -I../cybozulib/include mie_string_x64.o -o mie_string_test.exe

mie_string_x64.o: mie_string_x64.asm
	nasm -f elf64 mie_string_x64.asm

test: mie_string_test.exe
	./mie_string_test.exe

clean:
	rm -rf mie_string_test.exe mie_string_x64.o

