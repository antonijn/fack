
void foo(short a, char b)
{
	a = a * b;
}

int main()
{
	short a;
	char i, j;
	i = 10;
	j = 20;
	a = (i * j) * 2;
	foo(a, i);
}
