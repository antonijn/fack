[cpu 8086]
[bits 16]

i:
	resb 2

func:
	push bp
	mov bp, sp
	mov ax, word [ss:bp + 4]
	mov bx, word [i]
	mov word [ss:bp + 4], bx
	mov ax, word [ss:bp + 4]

.L0_0:
	mov ax, word [ss:bp + 4]
	mov bx, word [i]
	cmp ax, bx
	jge .L0_3
	jmp .L0_2

.L0_1:
	mov ax, word [i]
	mov bx, word [ss:bp + 4]
	mov word [i], bx
	mov ax, word [i]
	jmp .L0_0

.L0_2:
	mov ax, word [ss:bp + 4]
	mov bx, word [i]
	mov cx, word [ss:bp + 4]
	add bx, cx
	mov word [ss:bp + 4], bx
	mov ax, word [ss:bp + 4]
	jmp .L0_1

.L0_3:
	pop bp
	ret 
