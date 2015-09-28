# Make a bootable floppy image.
# Tested on Bochs.
#
#
SRC=./src
OBJ=./obj
BIN=./bin
PROG=omnius_boot
all: $(BIN)/floppy.img

$(BIN)/floppy.img: $(OBJ)/$(PROG).bin
	dd if=/dev/zero of="$@" bs=512 skip=1 count=2880
	dd if="$<" of="$@"

$(OBJ)/$(PROG).bin: $(OBJ)/$(PROG).o
	ld -Ttext 0x7c00 --oformat=binary $< -o $@

$(OBJ)/$(PROG).o: $(SRC)/$(PROG).S
	as $< -o $@

# C file	
#$(OBJ)/omnius_boot.o: $(SRC)/omnius_boot.c
#	gcc -c -g -Os -march=x86-64 -ffreestanding -Wall -Werror $(SRC)/omnius_boot.c -o $(OBJ)/omnius_boot.o
#
#$(OBJ)/omnius_boot.elf: $(SRC)/omnius_boot.o
#	ld -static -T$(SRC)/omnius_boot.ld -nostdlib --nmagic -o $(OBJ)/omnius_boot.elf $(OBJ)/omnius_boot.o
#
#$(BIN)/omnius_boot.bin: $(OBJ)/omnius_boot.elf
#	objcopy -O binary $(OBJ)/omnius_boot.elf $(OBJ)/omnius_boot.bin
#
clean: 
	rm -f $(OBJ)/$(PROG).o $(OBJ)/$(PROG).bin  $(BIN)/floppy.img
