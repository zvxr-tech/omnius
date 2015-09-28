# Bootloader for bochs x86 that will drop into protected mode (32bit).
#http://wiki.osdev.org/Protected_mode
#http://wiki.osdev.org/GDT_Tutorial
.code16                    #generate 16-bit code
.text                      #executable code location
.globl _start;
_start:                    #code entry point
	in $0x92, %al	# Enables A20 line
	test 2, %al
	jnz after_A20
	or 2, %al
	and $0xfe, %al
	out %al, $0x92
after_A20:

	movb $'X' , %al       #character to print
	movb $0x0e, %ah       #bios service code to print
	int  $0x10            #interrupt the cpu now

# This sets up the GDT and then far jumps into protected mode
set_gdtr:
    xor %eax, %eax
    mov %ds, %ax
    shl $0x04, %eax
    add $GDT, %eax
    xor %ebx, %ebx
    add $gdtr, %ebx
    add $0x02, %ebx
    mov %eax, (%ebx) # GDT_BASE = &GDT
    mov $GDT_end, %eax
    sub $GDT, %eax
    sub $0x1, %eax # GDT_LIMIT = size(GDT) - 1
    mov %ax, gdtr
    cli
    lgdt (gdtr)
    mov %cr0, %eax
    or 1, %al
    mov %eax, %cr0
    mov complete_flush, %ebx
    ljmp $0x08, $complete_flush
# 32 BIT MODE
.code32 
complete_flush:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
flush_loop:
    xor %eax, %eax
    jmp flush_loop

#gdtr: | LIMIT (16b) | BASE (32b) |
gdtr:.word 0 # For limit storage 16bits
     .word  0 # For base storage 16bits (1/2)
     .word  0 # For base storage 16bits (2/2)
# 
GDT: .quad  0x0000000000000000
     .quad  0x00CF9A000000FFFF
     .quad  0x00CF92000000FFFF
GDT_end:
     .quad  0x0000000000000000
. = _start + 510      #mov to 510th byte from 0 pos
.byte 0x55            #append boot signature
.byte 0xaa            #append boot signature
