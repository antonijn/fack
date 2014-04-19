
i:
	resb 2

func:
	push bp
	mov bp, sp
	cmp word [i], word [ss:bp + 4]
	jne .L0_0
	mov word [i], word [ss:bp + 4]
	jmp .L0_1

.L0_0:
	mov ax, word [i]
	add ax, word [ss:bp + 4]
	test ax, ax
	jz .L0_2
	mov word [ss:bp + 4], word [i]

.L0_2:

.L0_1:
	pop bp
	ret 
