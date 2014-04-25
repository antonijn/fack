[cpu 8086]
[bits 16]

section .bss
mode:
	resb 2

section .text
main:
	push bp
	mov bp, sp
	sub sp, 4
	mov ax, word [ss:bp + 10]
	mov word [ss:bp + 4], ax
	mov ax, word [ss:bp + 8]
	mov word [ss:bp + 2], ax
	call main
	add sp, 4
	pop bp
	ret
