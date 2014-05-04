[cpu 8086]
[bits 16]

section .bss
img:
	resb 18

mode:
	resb 2

section .text
enter13h:
	push esp
	mov ebp, esp

	; begin user generated asm

	mov ah, 0x0f
	int 0x10
	mov word [mode], ax
	xor ah, ah
	mov al, 0x13
	int 0x10
	
	; end user generated asm
	pop ebp
	ret

leave13h:
	push esp
	mov ebp, esp

	; begin user generated asm

	mov ax, word [mode]
	xor ah, ah
	int 0x10
	mov ah, 0x4c
	int 0x21
	
	; end user generated asm
	pop ebp
	ret

main:
	push esp
	mov ebp, esp
	sub esp, 4
	mov byte [ss:ebp + -3], 10
	mov byte [ss:ebp + -4], 20
	mov al, byte [ss:ebp + -3]
	mul byte [ss:ebp + -4]
	movsx eax, al
	mov ebx, 2
	mul ebx
	mov word [ss:ebp + -2], ax
	add esp, 4
	pop ebp
	ret
