/*
 * The paging kernel module walks through the IA-32e (x86-64) paging
 * structures to translate a linear address to a physical address.
 *
 * For educational purposes, the kernel structures are not used.
 * Page structures are manually parsed.
 *
 * Note: 2-MByte and 1-GByte pages are not supported by this kernel module.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/special_insns.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

struct linear_address {
	unsigned long offset:12;
	unsigned long table:9;
	unsigned long directory:9;
	unsigned long directory_ptr:9;
	unsigned long pml4:9;
	unsigned long sign:16;
};

static struct linear_address addr;
static unsigned long cr0, cr0_pg;
static unsigned long cr3;
static unsigned long cr4, cr4_pae, cr4_pcide;
static unsigned long efer, efer_lme;
static unsigned long maxphyaddr;
static unsigned long addr_mask;

static void parse_linear_address(unsigned long x)
{
	memcpy(&addr, &x, sizeof(unsigned long));

	pr_debug("linear address = 0x%lx\n", x);
	pr_debug("pml4 = %lu\n", (unsigned long)addr.pml4);
	pr_debug("directory_ptr = %lu\n", (unsigned long)addr.directory_ptr);
	pr_debug("directory = %lu\n", (unsigned long)addr.directory);
	pr_debug("table = %lu\n", (unsigned long)addr.table);
	pr_debug("offset = %lu\n", (unsigned long)addr.offset);
}

static void print_paging_structure(char *name, unsigned long *ps)
{
	int i;

	for (i = 0; i < 512; i++)
		if (ps[i] != 0)
			pr_debug("%s[%d] = 0x%lx\n", name, i, ps[i]);
}

static int ia32e_paging_enabled(void)
{
	cr0 = native_read_cr0();
	cr0_pg = cr0 >> 31 & 1;
	pr_debug("CR0 = 0x%lx CR0.PG = %lu\n", cr0, cr0_pg);

	cr4 = native_read_cr4();
	cr4_pae = cr4 >> 5 & 1;
	pr_debug("CR4 = 0x%lx CR4.PAE = %lu\n", cr4, cr4_pae);

	rdmsrl(MSR_EFER, efer);
	efer_lme = efer >> 8 & 1;
	pr_debug("EFER = 0x%lx EFER.LME = %lu\n", efer, efer_lme);

	if (!cr0_pg) {
		pr_debug("Paging is not enabled\n");
		return 0;
	} else if (!cr4_pae) {
		pr_debug("Physical Address Extension is not enabled\n");
		return 0;
	} else if (!efer_lme) {
		pr_debug("Long Mode is not enabled");
		return 0;
	}
	pr_debug("IA-32e paging enabled\n");

	return 1;
}

static void walkthrough(void)
{
	unsigned long *ps;
	void *page;

	cr3 = native_read_cr3();
	pr_debug("CR3 = 0x%lx\n", cr3);
	pr_debug("CR3 & addr_mask = 0x%lx\n", cr3 & addr_mask);
	ps = (unsigned long *)(PAGE_OFFSET + (cr3 & addr_mask));
	pr_debug("PAGE_OFFSET + CR3 & addr_mask = 0x%p\n", ps);

	print_paging_structure("PML4", ps);

	pr_debug("PML4E = 0x%lx\n", ps[addr.pml4]);
	pr_debug("PML4E.present = %lu\n", ps[addr.pml4] & 1);
	pr_debug("PML4E & addr_mask = 0x%lx\n", ps[addr.pml4] & addr_mask);
	ps = (unsigned long *)(PAGE_OFFSET + (ps[addr.pml4] & addr_mask));
	pr_debug("PAGE_OFFSET + PML4E & addr_mask = 0x%p\n", ps);

	print_paging_structure("PDPT", ps);

	pr_debug("PDPTE = 0x%lx\n", ps[addr.directory_ptr]);
	pr_debug("PDPTE.present = %lu\n", ps[addr.directory_ptr] & 1);
	pr_debug("PDPTE.pagesize = %lu\n", (ps[addr.directory_ptr] >> 7) & 1);
	pr_debug("PDPTE & addr_mask = 0x%lx\n", ps[addr.directory_ptr] & addr_mask);
	ps = (unsigned long *)(PAGE_OFFSET + (ps[addr.directory_ptr] & addr_mask));
	pr_debug("PAGE_OFFSET + PDPTE & addr_mask = 0x%p\n", ps);

	print_paging_structure("PD", ps);

	pr_debug("PDE = 0x%lx\n", ps[addr.directory]);
	pr_debug("PDE.present = %lu\n", ps[addr.directory_ptr] & 1);
	pr_debug("PDE.pagesize = %lu\n", (ps[addr.directory_ptr] >> 7) & 1);
	pr_debug("PDE & addr_mask = 0x%lx\n", ps[addr.directory] & addr_mask);
	ps = (unsigned long *)(PAGE_OFFSET + (ps[addr.directory] & addr_mask));
	pr_debug("PAGE_OFFSET + PDE & addr_mask = 0x%p\n", ps);

	print_paging_structure("PT", ps);

	pr_debug("PTE = 0x%lx\n", ps[addr.table]);
	pr_debug("PTE & addr_mask = 0x%lx\n", ps[addr.table] & addr_mask);
	page = (void *)(PAGE_OFFSET + (ps[addr.table] & addr_mask));
	pr_debug("PAGE_OFFSET + PTE & addr_mask = 0x%p\n", page);

	pr_debug("page[%lu] = 0x%lx\n", (unsigned long)addr.offset,
			*(unsigned long *)(page + addr.offset));
}

static ssize_t paging_write(struct file *filp, const char __user *ubuf,
				size_t count, loff_t *offp)
{
	ssize_t ret;
	unsigned long x;

	ret = simple_write_to_buffer(&x, sizeof(unsigned long),
					offp, ubuf, count);
	parse_linear_address(x);
	walkthrough();

	return ret;
}

static const struct file_operations paging_fops = {
	.owner = THIS_MODULE,
	.write = paging_write,
};

static struct miscdevice paging_dev = {
	.name = THIS_MODULE->name,
	.fops = &paging_fops,
};

static int __init paging_init(void)
{
	if (!ia32e_paging_enabled())
		return -EINVAL;

	cr4_pcide = (cr4 >> 17) & 1;
	pr_debug("CR4.PCIDE (process-context identifiers enable) = %lu\n",
		cr4_pcide);

	maxphyaddr = cpuid_eax(0x80000008) & ((1 << 8) - 1);
	pr_debug("maxphyaddr = %lu\n", maxphyaddr);

	addr_mask = (((unsigned long)1 << maxphyaddr) - 1) << 12;

	return misc_register(&paging_dev);
}

static void __exit paging_exit(void)
{
	misc_deregister(&paging_dev);
}

module_init(paging_init);
module_exit(paging_exit);

MODULE_LICENSE("GPL");
