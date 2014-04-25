
void * mode;

int main(int end)
{
	/* ajdiapwdjip */
	asm {
		mov ah, 0x0f
		int 0x10
		mov word [mode], ax
		
		xor ah, ah
		mov al, 0x13
		int 0x10
	}
	
	asm mov ax, word [mode];
	asm xor ah, ah;
	asm int 0x10;
	
	asm mov ah, 0x4c;
	asm int 0x21;
}
