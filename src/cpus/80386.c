#include <fack/cpus/8086.h>
#include <fack/cpus/80386.h>
#include <fack/codegen.h>
#include <fack/eparsingapi.h>

struct arch m80386 = {
	2,
	&eax, &ebx, &ecx, &edx,
	&esi, &edi, &esp, &ebp,
	
	&cdecl8086, NULL, NULL,
	
	0
};
