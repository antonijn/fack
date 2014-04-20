[cpu 8086]
[bits 16]

i:
	resb 2

func:
	push bp
	mov bp, sp
	mov ax, word [i]
	add ax, word [ss:bp + 4]
	mov bx, word [ss:bp + 4]
	add bx, word [i]
	cmp ax, bx
	jne .L0_0

.L0_0:
	pop bp
	ret 
