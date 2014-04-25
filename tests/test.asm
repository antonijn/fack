[cpu 8086]
[bits 16]

section .bss
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
	sub sp, 22
	mov word [ss:bp + -2], 0

.L0.0:
	cmp word [ss:bp + -2], 10
	jge .L0.3
	jmp .L0.2

.L0.1:
	mov ax, word [ss:bp + -2]
	add ax, 1
	mov word [ss:bp + -2], ax
	jmp .L0.0

.L0.2:
	sub sp, 2
	lea ax, word [ss:bp + -4]
	mov bx, ax
	add bx, word [ss:bp + -2]
	mov ax, word [bx]
	mov word [ss:bp + -24], ax
	add sp, 2
	jmp .L0.1

.L0.3:
	call enter13h
	call leave13h
	add sp, 22
	pop bp
	ret
