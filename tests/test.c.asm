
i:
	resb 2

func:
	push bp
	mov bp, sp
	cmp word [i], word [i]
	jne .L0_0
	jmp .L0_1

.L0_0:
	cmp word [ss:bp + 4], word [ss:bp + 4]
	jne .L0_2

.L0_2:

.L0_1:
	pop bp
	ret 
