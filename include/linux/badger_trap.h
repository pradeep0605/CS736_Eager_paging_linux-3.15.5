#ifndef _LINUX_BADGER_TRAP_H
#define _LINUX_BADGER_TRAP_H

#define MAX_NAME_LEN	16
#define PTE_RESERVED_MASK	(_AT(pteval_t, 1) << 51)

extern char badger_trap_process[CONFIG_NR_CPUS][MAX_NAME_LEN];
extern unsigned long base[256*1024];
extern unsigned long limit[256*1024];
extern unsigned long offset[256*1024];

extern int is_badger_trap_process(const char* proc_name);
extern inline pte_t pte_mkreserve(pte_t pte);
extern inline pte_t pte_unreserve(pte_t pte);
extern inline int is_pte_reserved(pte_t pte);
extern inline pmd_t pmd_mkreserve(pmd_t pmd);
extern inline pmd_t pmd_unreserve(pmd_t pmd);
extern inline int is_pmd_reserved(pmd_t pmd);
extern void badger_trap_init(struct mm_struct *mm);
extern inline void add_page_to_rmt(unsigned long virt, unsigned long phys);
extern inline void print_rmt(void);
extern inline int rmc_access(unsigned long address);
extern inline int vtlb_access(unsigned long address);
extern inline int ctlb_access(unsigned long address);
#endif /* _LINUX_BADGER_TRAP_H */
