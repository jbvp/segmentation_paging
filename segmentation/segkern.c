/*
 * The segkern module prints the following information:
 * - the code, stack and data segment selectors from the cs, ss and ds registers
 * - the Global Descriptor Table base and limit from the GDT register
 *   (retreived with the `sgdt` instruction)
 * - the Global Descriptor Table entries: the segment descriptors
 *
 * Note: Local Descriptor Tables are not supported.
 */
#include <linux/module.h>
#include <linux/init.h>
#include "segmentation.h"

static struct descriptor_table_register gdtr;
static unsigned int index;

module_param_named(base, gdtr.base, ulong, 0);
module_param_named(limit, gdtr.limit, ushort, 0);
module_param(index, uint, 0);

static void print_segment_selector(char *name, unsigned short ss)
{
	struct segment_selector *ssp;

	ssp = (struct segment_selector *)&ss;
	pr_debug("%s: Index=%hu Table Indicator=%hu Request Privilege Level=%hu\n",
		name, ssp->index, ssp->ti, ssp->rpl);
}

static void print_descriptor_table(struct segment_descriptor dt[], int i)
{
	struct segment_descriptor *entry = &dt[i];

	pr_debug("descriptor_table[%02d]=0x%016lx base=0x%lx G=%u limit=%lu S=%u type=%u DPL=%u\n",
		i, *(long unsigned int *)entry,
		BASE(entry),
		entry->g,
		LIMIT(entry),
		entry->s,
		entry->type,
		entry->dpl);
}

static int __init segkern_init(void)
{
	struct segment_descriptor *descriptor_table;
	int maxentries, i;

	if (gdtr.base == 0 || gdtr.limit == 0)
		store_gdtr(&gdtr);

	maxentries = (gdtr.limit + 1) / 8;

	pr_debug("gdtr.base = 0x%lx\n", gdtr.base);
	pr_debug("gdtr.limit = 0x%hx (max entries = %u)\n",
		gdtr.limit, maxentries);

	descriptor_table = (struct segment_descriptor *)gdtr.base;

	if (index) {

		print_descriptor_table(descriptor_table, index);

	} else {

		print_segment_selector("cs", get_cs());
		print_segment_selector("ss", get_ss());
		print_segment_selector("ds", get_ds());

		for (i = 0; i < maxentries; i++)
			print_descriptor_table(descriptor_table, i);

	}

	return 0;
}

static void __exit segkern_exit(void)
{

}

module_init(segkern_init);
module_exit(segkern_exit);

MODULE_LICENSE("GPL");
