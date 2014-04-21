[cpu 8086]
[bits 16]

i:
	resb 2

func:
	push bp
	mov bp, sp
	mov ax, word [i]
	mov bx, word [ss:bp + 4]
	mov cx, word [i]
	push ax
	mov ax, bx
	xor dx, dx
	div cx
	mov ax, word [ss:bp + 4]
	mov bx, ax
	xor dx, dx
	div bx
	mov word [i], ax
	mov ax, word [i]
	pop bp
	ret 
