
void * mode;

void enter13h()
{
	asm {
		mov ah, 0x0f
		int 0x10
		mov word [mode], ax
		
		xor ah, ah
		mov al, 0x13
		int 0x10
	}
}

void leave13h()
{
	asm {
		mov ax, word [mode]
		xor ah, ah
		int 0x10
		
		mov ah, 0x4c
		int 0x21
	}
}

int main(int e, int b)
{
	enter13h();
	
	leave13h();
}
