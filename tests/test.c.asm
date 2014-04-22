[cpu 8086]
[bits 16]

section .bss
i:
	resb 2

section .text
foo:
	push bp
	mov bp, sp

.L0.0:
	cmp word [i], word [ss:bp + 4]
	je .L0.1
	mov ax, word [i]
	add ax, 1
	mov word [i], ax
	jmp .L0.0

.L0.1:
	pop bp
	ret

bar:
	push bp
	mov bp, sp

.L0.0:
	mov ax, word [i]
	add ax, 1
	mov word [i], ax

.L0.1:
	cmp word [i], word [ss:bp + 4]
	jne .L0.0

.L0.2:
	pop bp
	ret

quux:
	push bp
	mov bp, sp
	mov word [i], 0

.L0.0:
	cmp word [i], word [ss:bp + 4]
	je .L0.3
	jmp .L0.2

.L0.1:
	mov ax, word [i]
	add ax, 1
	mov word [i], ax
	jmp .L0.0

.L0.2:
	jmp .L0.1

.L0.3:
	pop bp
	ret
