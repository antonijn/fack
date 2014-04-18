
i:
	resb 2

func:
	mov ax, word [i]
	sub ax, word [i]
	xor ax, -1
	test ax, ax
	jz .L0_0
	nop 

.L0_0:
	ret 
