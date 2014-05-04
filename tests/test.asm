[cpu 8086]
[bits 16]

section .bss
img:
	resb 18

mode:
	resb 2

section .text
enter13h:
	push bp
	mov bp, sp

	; begin user generated asm

	mov ah, 0x0f
	int 0x10
	mov word [mode], ax
	xor ah, ah
	mov al, 0x13
	int 0x10
	
	; end user generated asm
	pop bp
	ret

leave13h:
	push bp
	mov bp, sp

	; begin user generated asm

	mov ax, word [mode]
	xor ah, ah
	int 0x10
	mov ah, 0x4c
	int 0x21
	
	; end user generated asm
	pop bp
	ret

main:
	push bp
	mov bp, sp
	sub sp, 4
	mov al, byte [ss:bp + -3]
	mul byte [ss:bp + -4]
	mov bx, 2
	mul bx
	mov word [ss:bp + -2], ax
	add sp, 4
	pop bp
	ret
