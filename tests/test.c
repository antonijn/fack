
int a = 0x1, b = 0x10, c = 0x100, d;
int ptr[100];

void foo(int end)
{
	ptr[a] = b;
}

void bar(int end)
{
	do {
		a = (a + 1) * b;
	} while (a != end);
}

void quux(int end)
{
	for (a = 0; a != end; a = a + 1)
		;
}
