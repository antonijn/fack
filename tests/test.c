
int i;

void foo(int end)
{
	while (i != end) {
		i = i + 1;
	}
}

void bar(int end)
{
	do {
		i = i + 1;
	} while (i != end);
}

void quux(int end)
{
	for (i = 0; i != end; i = i + 1)
		;
}
