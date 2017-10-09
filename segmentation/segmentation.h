struct segment_selector {
	unsigned short rpl: 2, ti: 1, index: 13;
};

static unsigned short get_cs(void)
{
	unsigned short cs;

	asm volatile("mov %%cs, %0":"=r" (cs));
	return cs;
}

static unsigned short get_ss(void)
{
	unsigned short ss;

	asm volatile("mov %%ss, %0":"=r" (ss));
	return ss;
}

static unsigned short get_ds(void)
{
	unsigned short ds;

	asm volatile("mov %%ds, %0":"=r" (ds));
	return ds;
}

// From struct desc_ptr in arch/x86/include/asm/desc_defs.h
struct descriptor_table_register {
	unsigned short limit;
	unsigned long base;
}  __attribute__((packed));

static void store_gdtr(struct descriptor_table_register *dtr)
{
	asm volatile("sgdt %0":"=m" (*dtr));
}

// From truct desc_struct in arch/x86/include/asm/desc_defs.h
struct segment_descriptor {
	unsigned short limit0;
	unsigned short base0;
	unsigned short base1: 8, type: 4, s: 1, dpl: 2, p: 1;
	unsigned short limit1: 4, avl: 1, l: 1, d: 1, g: 1, base2: 8;
}  __attribute__((packed));

#define BASE(x) ((unsigned long)x->base0 | \
		((unsigned long)x->base1 << 16) | \
		((unsigned long)x->base2 << 24))
#define LIMIT(x) ((unsigned long)x->limit0 | \
		((unsigned long)x->limit1 << 16))
