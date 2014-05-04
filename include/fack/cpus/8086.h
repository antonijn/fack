#include <fack/options.h>
#include <fack/parsing.h>

extern struct cconv cdecl8086;
extern struct arch m8086;

void prepcall_cdecl8086(FILE * ofile, struct cfunctiontype * cft);
void cleancall_cdecl8086(FILE * ofile, struct cfunctiontype * cft);
void call_cdecl8086(FILE * ofile, void * cft);
void loadparam_cdecl8086(FILE * ofile, struct cfunctiontype * cft, int param, void * p);
void * retrieveret_cdecl8086(FILE * ofile, struct cfunctiontype * cft);
void * retrieveparam_cdecl8086(FILE * ofile, struct cfunctiontype * cft, int param);
void loadret_cdecl8086(FILE * ofile, struct cfunctiontype * cft, void * ret);
