[cpu 386]
[bits 32]

section .text
foo:
	push esp
	mov ebp, esp
	mov eax, dword [ss:ebp + 8]
	mul dword [ss:ebp + 12]
	mov dword [ss:ebp + 8], eax
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
	sub esp, 8
	mov ax, word [ss:ebp + -2]
	movsx eax, ax
	mov dword [ss:bp + -8], eax
	mov al, byte [ss:ebp + -3]
	movsx eax, al
	mov dword [ss:bp + -12], eax
	call foo
	add esp, 8
	add esp, 4
	pop ebp
	ret
