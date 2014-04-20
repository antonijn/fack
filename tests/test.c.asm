[cpu 8086]
[bits 16]

i:
	resb 2

func:
	push bp
	mov bp, sp
	mov ax, word [i]
	add ax, word [ss:bp + 4]
	add ax, word [i]
	add ax, word [ss:bp + 4]
	mov bx, word [ss:bp + 4]
	add bx, word [i]
	add bx, 10
	add bx, 16
	add bx, 8
	cmp ax, bx
	jne .L0_0
	mov word [ss:bp + 4], word [i]

.L0_0:
	pop bp
	ret 
