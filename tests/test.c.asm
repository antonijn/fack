[cpu 8086]
[bits 16]

i:
	resb 2

foo:
	push bp
	mov bp, sp

.L0.0:
	mov ax, word [i]
	mov bx, word [ss:bp + 4]
	cmp ax, bx
	je .L0.1
	mov ax, word [i]
	mov bx, word [i]
	add bx, 1
	mov word [i], bx
	mov ax, word [i]
	jmp .L0.0

.L0.1:
	pop bp
	ret

bar:
	push bp
	mov bp, sp

.L0.0:
	mov ax, word [i]
	mov bx, word [i]
	add bx, 1
	mov word [i], bx
	mov ax, word [i]

.L0.1:
	mov ax, word [i]
	mov bx, word [ss:bp + 4]
	cmp ax, bx
	jne .L0.0

.L0.2:
	pop bp
	ret

quux:
	push bp
	mov bp, sp

.L0.0:
	mov ax, word [i]
	mov bx, word [ss:bp + 4]
	cmp ax, bx
	je .L0.3
	jmp .L0.2

.L0.1:
	mov ax, word [i]
	mov bx, word [i]
	add bx, 1
	mov word [i], bx
	mov ax, word [i]
	jmp .L0.0

.L0.2:
	jmp .L0.1

.L0.3:
	pop bp
	ret
