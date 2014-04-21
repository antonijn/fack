[cpu 8086]
[bits 16]

i:
	resb 2

func:
	push bp
	mov bp, sp
	mov ax, word [ss:bp + 4]
	mov bx, word [i]
	xor dx, dx
	div bx
	mov ax, word [ss:bp + 4]
	push ax
	mov ax, dx
	pop bx
	mul bx
	pop bp
	ret 
