#include <stdio.h>
#include "segmentation.h"

static void print_segment_selector(char *name, unsigned short ss)
{
	struct segment_selector *ssp;

	ssp = (struct segment_selector *)&ss;
	printf("%s: Index=%hu Table Indicator=%hu Request Privilege Level=%hu\n",
		name, ssp->index, ssp->ti, ssp->rpl);
}

int main(int argc, char *argv[])
{
	struct descriptor_table_register gdtr;
	unsigned long *p;
	unsigned long localvar = 0x1337133713371337;
	int maxentries;

	store_gdtr(&gdtr);

	printf("&localvar = %p\n", &localvar);

	maxentries = (gdtr.limit + 1) / 8;

	printf("gdtr.base = 0x%lx\n", gdtr.base);
	printf("gdtr.limit = 0x%hx (max entries = %u)\n",
		gdtr.limit, maxentries);

	print_segment_selector("cs", get_cs());
	print_segment_selector("ss", get_ss());
	print_segment_selector("ds", get_ds());
}
