#ifndef _LINUX_BADGER_TRAP_H
#define _LINUX_BADGER_TRAP_H

#define MAX_NAME_LEN	16
#define PTE_RESERVED_MASK	(_AT(pteval_t, 1) << 51)

char badger_trap_process[CONFIG_NR_CPUS][MAX_NAME_LEN];
unsigned long base[256*1024];
unsigned long limit[256*1024];
unsigned long offset[256*1024];

int is_badger_trap_process(const char* proc_name);
inline pte_t pte_mkreserve(pte_t pte);
inline pte_t pte_unreserve(pte_t pte);
inline int is_pte_reserved(pte_t pte);
inline pmd_t pmd_mkreserve(pmd_t pmd);
inline pmd_t pmd_unreserve(pmd_t pmd);
inline int is_pmd_reserved(pmd_t pmd);
void badger_trap_init(struct mm_struct *mm);
inline void add_page_to_rmt(unsigned long virt, unsigned long phys);
inline void print_rmt(void);
inline int rmc_access(unsigned long address);
inline int vtlb_access(unsigned long address);
inline int ctlb_access(unsigned long address);
#endif /* _LINUX_BADGER_TRAP_H */
