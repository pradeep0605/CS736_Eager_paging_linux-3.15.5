#include <asm/pgalloc.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <linux/badger_trap.h>
#include <linux/syscalls.h>
#include <linux/hugetlb.h>
#include <linux/kernel.h>


/*
 * This syscall is generic way of setting up badger trap. 
 * There are three options to start badger trap.
 * (1) 	option > 0: provide all process names with number of processes.
 * 	This will mark the process names for badger trap to start when any
 * 	process with names specified will start.
 *
 * (2) 	option == 0: starts badger trap for the process calling the syscall itself.
 *  	This requires binary to be updated for the workload to call badger trap. This
 *  	option is useful when you want to skip the warmup phase of the program. You can 
 *  	introduce the syscall in the program to invoke badger trap after that phase.
 *
 * (3) 	option < 0: provide all pid with number of processes. This will start badger
 *  	trap for all pids provided immidiately.
 *
 *  Note: 	(1) will allow all the child processes to be marked for badger trap when
 *  		forked from a badger trap process.

 *		(2) and (3) will not mark the already spawned child processes for badger 
 *		trap when you mark the parent process for badger trap on the fly. But (2) and (3) 
 *		will mark all child spwaned from the parent process adter being marked for badger trap. 
 */
SYSCALL_DEFINE3(init_badger_trap, const char __user**, process_name, unsigned long, num_procs, int, option)
{
	unsigned int i;
	char *temp;
	unsigned long ret=0;
	char proc[MAX_NAME_LEN];
	struct task_struct * tsk;
	unsigned long pid;

	if(option > 0)
	{
		for(i=0; i<CONFIG_NR_CPUS; i++)
		{
			if(i<num_procs)
				ret = strncpy_from_user(proc, process_name[i], MAX_NAME_LEN);
			else
				temp = strncpy(proc,"",MAX_NAME_LEN);
			temp = strncpy(badger_trap_process[i], proc, MAX_NAME_LEN-1);
		}
	}

	// All other inputs ignored
	if(option == 0)
	{
		current->mm->badger_trap_en = 1;
		badger_trap_init(current->mm);
	}

	if(option < 0)
	{
		for(i=0; i<CONFIG_NR_CPUS; i++)
		{
			if(i<num_procs)
			{
				ret = kstrtoul(process_name[i],10,&pid);
				if(ret == 0)
				{
					tsk = find_task_by_vpid(pid);
					tsk->mm->badger_trap_en = 1;
					badger_trap_init(tsk->mm);
				}
			}
		}
	}

	for(i=0; i<(256*1024); i++)
	{
		base[i] = 0;
		limit[i] = 0;
		offset[i] = 0;
	}

	return 0;
}

/*
 * This function checks whether a process name provided matches from the list
 * of process names stored to be marked for badger trap.
 */
int is_badger_trap_process(const char* proc_name)
{
	unsigned int i;
	for(i=0; i<CONFIG_NR_CPUS; i++)
	{
		if(!strncmp(proc_name,badger_trap_process[i],MAX_NAME_LEN))
			return 1;
	}
	return 0;
}

/*
 * Helper functions to manipulate all the TLB entries for reservation.
 */
inline pte_t pte_mkreserve(pte_t pte)
{
        return pte_set_flags(pte, PTE_RESERVED_MASK);
}

inline pte_t pte_unreserve(pte_t pte)
{
        return pte_clear_flags(pte, PTE_RESERVED_MASK);
}

inline int is_pte_reserved(pte_t pte)
{
        if(native_pte_val(pte) & PTE_RESERVED_MASK)
                return 1;
        else
                return 0;
}

inline pmd_t pmd_mkreserve(pmd_t pmd)
{
        return pmd_set_flags(pmd, PTE_RESERVED_MASK);
}

inline pmd_t pmd_unreserve(pmd_t pmd)
{
        return pmd_clear_flags(pmd, PTE_RESERVED_MASK);
}

inline int is_pmd_reserved(pmd_t pmd)
{
        if(native_pmd_val(pmd) & PTE_RESERVED_MASK)
                return 1;
        else
                return 0;
}

/*
 * This function walks the page table of the process being marked for badger trap
 * This helps in finding all the PTEs that are to be marked as reserved. This is 
 * espicially useful to start badger trap on the fly using (2) and (3). If we do not
 * call this function, when starting badger trap for any process, we may miss some TLB 
 * misses from being tracked which may not be desierable.
 *
 * Note: This function takes care of transparent hugepages and hugepages in general.
 */
void badger_trap_init(struct mm_struct *mm)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	pte_t *page_table;
	spinlock_t *ptl;
	unsigned long address;
	unsigned long i,j,k,l;
	unsigned long user = 0;
	unsigned long mask = _PAGE_USER | _PAGE_PRESENT;
	struct vm_area_struct *vma;
	pgd_t *base = mm->pgd;
	for(i=0; i<PTRS_PER_PGD; i++)
	{
		pgd = base + i;
		if((pgd_flags(*pgd) & mask) != mask)
			continue;
		for(j=0; j<PTRS_PER_PUD; j++)
		{
			pud = (pud_t *)pgd_page_vaddr(*pgd) + j;
			if((pud_flags(*pud) & mask) != mask)
                        	continue;
			address = (i<<PGDIR_SHIFT) + (j<<PUD_SHIFT);
			vma = find_vma(mm, address);
			if(vma && pud_huge(*pud) && is_vm_hugetlb_page(vma))
			{
				spin_lock(&mm->page_table_lock);
				page_table = huge_pte_offset(mm, address);
				*page_table = pte_mkreserve(*page_table);
				spin_unlock(&mm->page_table_lock);
				continue;
			}
			for(k=0; k<PTRS_PER_PMD; k++)
			{
				pmd = (pmd_t *)pud_page_vaddr(*pud) + k;
				if((pmd_flags(*pmd) & mask) != mask)
					continue;
				address = (i<<PGDIR_SHIFT) + (j<<PUD_SHIFT) + (k<<PMD_SHIFT);
				vma = find_vma(mm, address);
				if(vma && pmd_huge(*pmd) && (transparent_hugepage_enabled(vma)||is_vm_hugetlb_page(vma)))
				{
					ptl = pmd_lock(mm,pmd);
					*pmd = pmd_mkreserve(*pmd);
					spin_unlock(ptl);
					continue;
				}
				for(l=0; l<PTRS_PER_PTE; l++)
				{
					pte = (pte_t *)pmd_page_vaddr(*pmd) + l;
					if((pte_flags(*pte) & mask) != mask)
						continue;
					address = (i<<PGDIR_SHIFT) + (j<<PUD_SHIFT) + (k<<PMD_SHIFT) + (l<<PAGE_SHIFT);
					vma = find_vma(mm, address);
					if(vma)
					{
						page_table = pte_offset_map_lock(mm, pmd, address, &ptl);
						*pte = pte_mkreserve(*pte);
						pte_unmap_unlock(page_table, ptl);
						add_page_to_rmt(address,(pte_val(*pte) & PTE_PFN_MASK));
					}
					user++;
				}
			}
		}
	}
}

inline void add_page_to_rmt(unsigned long virt, unsigned long phys)
{
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;
	struct vm_area_struct *vma;
	static DEFINE_SPINLOCK(rmt_spinlock);
	unsigned long i;
	unsigned long j;
	unsigned long neg_stride = 0;
	unsigned long pos_stride = 0;
	unsigned long base_new = virt;
	unsigned long limit_new = virt+PAGE_SIZE;
	long offset_new = 0;
//	unsigned long last_deadbeef = 0;
	int extend = 0;
	int conflict = 0;
	unsigned long phys_start = 0;
        unsigned long mask = _PAGE_USER | _PAGE_PRESENT;


	vma = find_vma(current->mm, virt);

	spin_lock(&rmt_spinlock);
//	if(vma->apriori_en != 1)
//		printk("VMA: Start: %lx End: %lx en: %d Flags: %lx\n",vma->vm_start, vma->vm_end, vma->apriori_en, vma->vm_flags);

	//1st optimization
	//Do a simple check if we can extend the RMT entry (should be only 1)
	for(i=0; i<256*1024; i++)
        {
                if(base[i]==0 && limit[i]==0)
                        break;
                else if((base[i]-PAGE_SIZE == virt && offset[i]-PAGE_SIZE==phys) || (limit[i]==virt && (offset[i]+limit[i]-base[i])==phys))
			extend++;
        }
	//No case of merging or splitting as of now... simply extend
	if(extend==1)
	{
        	for(i=0; i<256*1024; i++)
        	{
                	if(base[i]==0 && limit[i]==0)
                	        break;
                	else if(base[i]-PAGE_SIZE == virt && offset[i]-PAGE_SIZE==phys)
			{
				base[i] = base[i]-PAGE_SIZE;
				offset[i] = phys;
				spin_unlock(&rmt_spinlock);
				return;
			}
			else if(limit[i]==virt && (offset[i]+limit[i]-base[i])==phys)
			{
				limit[i] = limit[i]+PAGE_SIZE;
				spin_unlock(&rmt_spinlock);
				return;
			}
        	}
	}


	//Optimization 2: Create a new RMT entry by walking the page tables around the newly allocated page
	//Remember that this RMT entry should not overlap with any other entry in the RMT, if it does then
	//recreate the RMT entries for this VMA
        for(i=(virt-PAGE_SIZE); i>=vma->vm_start; i=i-PAGE_SIZE)
        {
                pgd = pgd_offset(current->mm, i);
		if((pgd_flags(*pgd) & mask) != mask)
			break;
                pud = pud_offset(pgd, i);
		if((pud_flags(*pud) & mask) != mask)
			break;
                pmd = pmd_offset(pud, i);
		if((pmd_flags(*pmd) & mask) != mask)
			break;
                pte = pte_offset_map(pmd, i);
		if((pte_flags(*pte) & mask) != mask)
			break;
                if((phys - (pte_val(*pte) & PTE_PFN_MASK)) == (virt - i))
                        neg_stride++;
                else
                        break;
        }
        for(i=(virt+PAGE_SIZE); i<vma->vm_end; i=i+PAGE_SIZE)
        {
                pgd = pgd_offset(current->mm, i);
		if((pgd_flags(*pgd) & mask) != mask)
			break;
                pud = pud_offset(pgd, i);
		if((pud_flags(*pud) & mask) != mask)
			break;
                pmd = pmd_offset(pud, i);
		if((pmd_flags(*pmd) & mask) != mask)
			break;
                pte = pte_offset_map(pmd, i);
		if((pte_flags(*pte) & mask) != mask)
			break;
                if(((pte_val(*pte) & PTE_PFN_MASK)-phys) == (i - virt))
                        pos_stride++;
                else
                        break;
        }
	if(pos_stride+neg_stride+1>8)
	{
		base_new = virt - neg_stride*PAGE_SIZE;
		limit_new = virt + (pos_stride+1)*PAGE_SIZE;
		offset_new = phys - neg_stride*PAGE_SIZE;
		//Check for any conflict in RMT, otherwise it is ideal to add this to RMT directly
                for(i=0; i<256*1024; i++)
                {
                        if(base[i]==0 && limit[i]==0)
                                break;
			else if((base[i]<=base_new && base_new<limit[i]) || (base[i]<limit_new && limit_new<=limit[i]) || (base_new<=base[i] && base[i]<limit_new) || (base_new<limit[i] && limit[i]<=limit_new))
				conflict++;
		}
		if(conflict==0)
		{
                	for(i=0; i<256*1024; i++)
                	{
                        	if((base[i]==0 && limit[i]==0) || (base[i]==0xdeadbeef && limit[i]==0xdeadbeef))
                        	{
					base[i] = base_new;
					limit[i] = limit_new;
					offset[i] = offset_new;
	                                spin_unlock(&rmt_spinlock);
        	                        return;
				}
                	}
		}
	}

	//Find all the entries in the RMT of this VMA and invalidate them
	for(i=0; i<256*1024; i++)
	{
		if(base[i]==0 && limit[i]==0)
			break;
		if(vma->vm_start<=base[i] && (limit[i]-PAGE_SIZE)<=vma->vm_end)
		{
			base[i] = 0xdeadbeef;
			limit[i] = 0xdeadbeef;
			offset[i] = 0xdeadbeef;
		}
	}

	//Find all RMT entries in this VMA
	for(i=vma->vm_start; i<vma->vm_end;)
	{
		pos_stride = 0;
		base_new = 0;
		limit_new = 0;
		offset_new = 0;
		phys_start = 0;
	        for(j=i; j<vma->vm_end; j=j+PAGE_SIZE)
	        {
        	        pgd = pgd_offset(current->mm, j);
			if((pgd_flags(*pgd) & mask) != mask)
			{
				j = (j & PGDIR_MASK) + PGDIR_SIZE - PAGE_SIZE;	
				break;
			}
        	        pud = pud_offset(pgd, j);
			if((pud_flags(*pud) & mask) != mask)
			{
				j = (j & PUD_MASK) + PUD_SIZE - PAGE_SIZE;
				break;
			}
        	        pmd = pmd_offset(pud, j);
			if((pmd_flags(*pmd) & mask) != mask)
			{
				j=(j & PMD_MASK) + PMD_SIZE - PAGE_SIZE;
				break;
			}
        	        pte = pte_offset_map(pmd, j);
			if((pte_flags(*pte) & mask) != mask)
				break;
			if(i==j)
			{
				phys_start = (pte_val(*pte) & PTE_PFN_MASK);
				continue;
			}
        	        else if(((pte_val(*pte) & PTE_PFN_MASK)-phys_start) == (j - i))
        	        {
        	                pos_stride++;
				base_new = i;
        	                limit_new = j+PAGE_SIZE;
        	                offset_new = phys_start;
        	        }
        	        else
			{
				j = j-PAGE_SIZE;
                	        break;
			}
        	}	
		i = j+PAGE_SIZE;
		//Found a range
		if(pos_stride >= 8)
		{
			//Add to RMT	
	                for(j=0; j<256*1024; j++)
        	        {
				if((base[j]==0 && limit[j]==0) || (base[j]==0xdeadbeef && limit[j]==0xdeadbeef))
				{
					base[j]=base_new;
					limit[j]=limit_new;
					offset[j]=offset_new;
					break;
				}
			}
		}
	}
	spin_unlock(&rmt_spinlock);
/*

	//Find how long is the range in backward direction first
	for(i=(virt-PAGE_SIZE); i>=vma->vm_start; i=i-PAGE_SIZE)
	{
		pgd = pgd_offset(current->mm, i);
		pud = pud_offset(pgd, i);
		pmd = pmd_offset(pud, i);
		pte = pte_offset_map(pmd, i);
		if((phys - (pte_val(*pte) & PTE_PFN_MASK)) == (virt - i))
		{
			neg_stride++;
			base_new = i;
			offset_new = virt-phys;
		}
		else
			break;
	}
	//Find how long is the range in forward direction
	for(i=(virt+PAGE_SIZE); i<vma->vm_end; i=i+PAGE_SIZE)
	{
		pgd = pgd_offset(current->mm, i);
		pud = pud_offset(pgd, i);
		pmd = pmd_offset(pud, i);
		pte = pte_offset_map(pmd, i);
		if(((pte_val(*pte) & PTE_PFN_MASK)-phys) == (i - virt))
		{
			pos_stride++;
			limit_new = i+PAGE_SIZE;
			offset_new = virt-phys;
		}
		else
			break;
	}
	if(pos_stride+neg_stride+1>1)
	{
		spin_lock(&rmt_spinlock);
		//Check if the range already exists in the RMT
		for(i=0; i<256*1024; i++)
		{
			if(base[i]==0 && limit[i]==0 && found==1)
				break;
			else if (base[i]==0 && limit[i]==0 && found==0)
			{
				if(last_deadbeef)
				{
					base[last_deadbeef] = base_new;
					limit[last_deadbeef] = limit_new;
					offset[last_deadbeef] = offset_new;
				}
				else
				{
					base[i] = base_new;
					limit[i] = limit_new;
					offset[i] = offset_new;
				}
				break;
			}
			else if((base[i]<=base_new && base_new<limit[i]) || (base[i]<limit_new && limit_new<=limit[i]) || (base_new<=base[i] && base[i]<limit_new) || (base_new<limit[i] && limit[i]<=limit_new))
			{
				if(found==0)
				{
					base[i] = base_new;
					limit[i] = limit_new;
					offset[i] = offset_new;
				}
				else
				{
					base[i] = 0xdeadbeef;
					limit[i] = 0xdeadbeef;
					offset[i] = 0xdeadbeef;
					last_deadbeef = i;
				}
				found = 1;
			}
//			printk("RMT[%lu]: %lx %lx\n",i,base[i],limit[i]);
		}
//		printk("VMA Virt: %lx Phys: %lx Start: %lx End: %lx neg: %lu pos: %lu\n",virt, phys, vma->vm_start, vma->vm_end,neg_stride,pos_stride);
		spin_unlock(&rmt_spinlock);
	}
*/
}

inline void print_rmt(void)
{
	unsigned long i;
	unsigned long count = 0;
	for(i=0; i<256*1024; i++)
	{
		if(base[i]==0 && limit[i]==0)
			break;
		else if(base[i]!=0xdeadbeef && limit[i]!=0xdeadbeef)
		{
			printk("RMT[%lu]: %lx %lx len: %lu offset: %lx\n",i,base[i],limit[i], (limit[i]-base[i])>>PAGE_SHIFT, offset[i]);
			count++;
		}
	}
	printk("Total RMT entries: %lu\n",count);
}

inline int ctlb_access(unsigned long address)
{
	unsigned long i;
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;
	unsigned long bits;
	unsigned long virt;
	unsigned long entry;
	unsigned long phys;
	unsigned int count = 0;
	unsigned long min =  0xFFFFFFFFFFFFFFFF;
	unsigned int ctlb_hit = 0;
	unsigned long lru = 0;
	unsigned long mask = _PAGE_USER | _PAGE_PRESENT;

	virt = address & 0xFFFFFFFFFFFF8000;
	bits = 1 << ((address >> PAGE_SHIFT) & 0x7);
	entry = virt | bits;
	//Check for a CTLB hit
	for(i=0; i<512; i++)
	{
		if((current->ctlb_entry[i] & entry) == entry)
		{
			current->ctlb_lru[i] = current->total_dtlb_misses;
			ctlb_hit = 1;
			break;
		}
		if(current->ctlb_lru[i]<min)
			lru = i;
	}
	if(ctlb_hit==1)
	{
		current->ctlb_hits++;
		return 1;
	}
	current->ctlb_misses++;

	//CTLB Miss for sure
	//Create a CTLB Entry
        pgd = pgd_offset(current->mm, address);
	if((pgd_flags(*pgd) & mask) != mask)
                        return 0;
        pud = pud_offset(pgd, address);
	if((pud_flags(*pud) & mask) != mask)
                        return 0;
        pmd = pmd_offset(pud, address);
	if((pmd_flags(*pmd) & mask) != mask)
                        return 0;
        pte = pte_offset_map(pmd, address);

	phys = (pte_val(*pte) & PTE_PFN_MASK) & 0xFFFFFFFFFFFF8000;
	if((pte_flags(*pte) & mask) != mask)
                        return 0;

	for(i=virt; i<(virt+8*PAGE_SIZE); i=i+PAGE_SIZE)
	{
		pte = pte_offset_map(pmd, i);
		if(phys == ((pte_val(*pte) & PTE_PFN_MASK) & 0xFFFFFFFFFFFF8000))
		{
			count++;
			entry = entry | (1 << ((i >> PAGE_SHIFT) & 0x7));
		}
	}
	if(count>=2)
	{
		current->ctlb_entry[lru] = entry;
		current->ctlb_lru[lru] = current->total_dtlb_misses;
	}
	return 0;
	
}

//This function does all the virtual TLB accesses
inline int vtlb_access(unsigned long address)
{
	struct vm_area_struct *vma;
	unsigned long i;
	unsigned int vtlb_hit[4] = {0,0,0,0};
	unsigned int vtlb_lru[4] = {0,0,0,0};
	unsigned long min = 0xFFFFFFFFFFFFFFFF;

	//Find Infinite Virtual TLB hits
	vma = find_vma(current->mm, address);
	if((vma->vm_end-vma->vm_start)<8)
		return 0;

/*	if(current->vtlb_hit_base[0]<=address && address<current->vtlb_hit_limit[0])
	{
		vtlb_hit[0] = 1;
		current->vtlb_buffer_hits[0]++;
	}
	if(vtlb_hit[0] == 0)
	{
	        for(i=0; i<16; i++)
	        {
	                if(current->vtlb_base[0][i]<=address && address<current->vtlb_limit[0][i])
	                {
	                        vtlb_hit[0] = 1;
	                        current->vtlb_lru[0][i] = current->total_dtlb_misses;
	                }
	                if(current->vtlb_lru[0][i]<min)
	                {
	                        vtlb_lru[0] = i;
	                        min = current->vtlb_lru[0][i];
	                }
	        }
        	if(vtlb_hit[0]==1)
        	        current->vtlb_hits[0]++;
        	else
        	        current->vtlb_misses[0]++;
	}

	if(current->vtlb_hit_base[1]<=address && address<current->vtlb_hit_limit[1])
	{
		vtlb_hit[1] = 1;
		current->vtlb_buffer_hits[1]++;
	}
	if(vtlb_hit[1] == 0)
	{
	        min = 0xFFFFFFFFFFFFFFFF;
	        for(i=0; i<32; i++)
	        {
	                if(current->vtlb_base[1][i]<=address && address<current->vtlb_limit[1][i])
	                {
	                        vtlb_hit[1] = 1;
	                        current->vtlb_lru[1][i] = current->total_dtlb_misses;
	                }
	                if(current->vtlb_lru[1][i]<min)
	                {
	                        vtlb_lru[1] = i;
	                        min = current->vtlb_lru[1][i];
	                }
	        }
        	if(vtlb_hit[1]==1)
        	        current->vtlb_hits[1]++;
        	else
        	        current->vtlb_misses[1]++;
	}

	if(current->vtlb_hit_base[2]<=address && address<current->vtlb_hit_limit[2])
	{
		vtlb_hit[2] = 1;
		current->vtlb_buffer_hits[2]++;
	}
	if(vtlb_hit[2] == 0)
	{
	        min = 0xFFFFFFFFFFFFFFFF;
	        for(i=0; i<64; i++)
	        {
	                if(current->vtlb_base[2][i]<=address && address<current->vtlb_limit[2][i])
	                {
	                        vtlb_hit[2] = 1;
	                        current->vtlb_lru[2][i] = current->total_dtlb_misses;
	                }
	                if(current->vtlb_lru[2][i]<min)
	                {
	                        vtlb_lru[2] = i;
	                        min = current->vtlb_lru[2][i];
	                }
	        }
	        if(vtlb_hit[2]==1)
	                current->vtlb_hits[2]++;
	        else
	                current->vtlb_misses[2]++;
	}

	if(current->vtlb_hit_base[3]<=address && address<current->vtlb_hit_limit[3])
	{
		vtlb_hit[3] = 1;
		current->vtlb_buffer_hits[3]++;
	}
	if(vtlb_hit[3] == 0)
	{
	        min = 0xFFFFFFFFFFFFFFFF;
	        for(i=0; i<128; i++)
	        {
	                if(current->vtlb_base[3][i]<=address && address<current->vtlb_limit[3][i])
	                {
	                        vtlb_hit[3] = 1;
	                        current->vtlb_lru[3][i] = current->total_dtlb_misses;
	                }
	                if(current->vtlb_lru[3][i]<min)
	                {
	                        vtlb_lru[3] = i;
	                        min = current->vtlb_lru[3][i];
	                }
	        }
	        if(vtlb_hit[3]==1)
	                current->vtlb_hits[3]++;
	        else
	                current->vtlb_misses[3]++;
	}
	
        for(i=0; i<4; i++)
        {
                if(vtlb_hit[i]==0)
                {
                        current->vtlb_base[i][(vtlb_lru[i])] = vma->vm_start;
                        current->vtlb_limit[i][(vtlb_lru[i])] = vma->vm_end;
                        current->vtlb_lru[i][(vtlb_lru[i])] = current->total_dtlb_misses;
			current->vtlb_hit_base[i] = vma->vm_start;
			current->vtlb_hit_limit[i] = vma->vm_end;
                }
        }
*/	
	return 1;
	
}

// This functions does all the RMC access and figures out if the TLB miss is covered by RMT or not
inline int rmc_access(unsigned long address)
{

	//Remember RMC is per-thread already. So no locks required here.
	unsigned long i,j;
	unsigned int rmc_hit[7] = {0,0,0,0,0,0,0};
	unsigned int rmc_lru[7] = {0,0,0,0,0,0,0};
	unsigned long min = 0xFFFFFFFFFFFFFFFF;
	unsigned int rmt_hit = 0;
	unsigned long found_base=0;
	unsigned long found_limit=0;
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;

	
	struct vm_area_struct *vma = find_vma(current->mm, address); 
	//Find RMT hits
	for(i=0; i<256*1024; i++)
        {
                if(base[i]==0 && limit[i]==0)
                        break;
                else if(base[i]!=0xdeadbeef && limit[i]!=0xdeadbeef)
                {
                        if(base[i]<=address && address<limit[i])
                        {
                                rmt_hit= 1;
                                found_base = base[i];
                                found_limit = limit[i];
                                break;
                        }
                }
        }

	if(rmt_hit==0)
	{
                pgd = pgd_offset(current->mm, address);
                pud = pud_offset(pgd, address);
                pmd = pmd_offset(pud, address);
                pte = pte_offset_map(pmd, address);
		add_page_to_rmt((address & PAGE_MASK),(pte_val(*pte) & PTE_PFN_MASK));
//		printk("RMT miss @ %lx: VMA: %lx %lx %d\n",address,vma->vm_start, vma->vm_end, vma->apriori_en);
		return 0;
        }


/*	if(current->rmc_hit_base[0]<=address && address<current->rmc_hit_limit[0])
	{
		rmc_hit[0] = 1;
		current->rmc_buffer_hits[0]++;
	}
	if(rmc_hit[0] == 0)
	{
		//Check in each RMC for hit or miss
		for(i=0; i<16; i++)
		{
			if(current->rmc_base[0][i]<=address && address<current->rmc_limit[0][i])
			{
				rmc_hit[0] = 1;
				current->rmc_lru[0][i] = current->total_dtlb_misses;
				current->rmc_hit_base[0] = current->rmc_base[0][i];
				current->rmc_hit_limit[0] = current->rmc_limit[0][i];
			}
			if(current->rmc_lru[0][i]<min)
			{
				rmc_lru[0] = i;
				min = current->rmc_lru[0][i];
			}
		}
		if(rmc_hit[0]==1)
			current->rmc_hits[0]++;
		else
		{
			current->rmc_misses[0]++;
		}
	}
	if(current->rmc_hit_base[1]<=address && address<current->rmc_hit_limit[1])
	{
		rmc_hit[1] = 1;
		current->rmc_buffer_hits[1]++;
	}
	if(rmc_hit[1] == 0)
	{
		//Check in each RMC for hit or miss
		min = 0xFFFFFFFFFFFFFFFF;
		for(i=0; i<32; i++)
		{
			if(current->rmc_base[1][i]<=address && address<current->rmc_limit[1][i])
			{
				rmc_hit[1] = 1;
				current->rmc_lru[1][i] = current->total_dtlb_misses;
				current->rmc_hit_base[1] = current->rmc_base[1][i];
				current->rmc_hit_limit[1] = current->rmc_limit[1][i];
			}
			if(current->rmc_lru[1][i]<min)
			{
				rmc_lru[1] = i;
				min = current->rmc_lru[1][i];
			}
		}
		if(rmc_hit[1]==1)
			current->rmc_hits[1]++;
		else
		{
			current->rmc_misses[1]++;
		}
	}
	if(current->rmc_hit_base[2]<=address && address<current->rmc_hit_limit[2])
	{
		rmc_hit[2] = 1;
		current->rmc_buffer_hits[2]++;
	}
	if(rmc_hit[2] == 0)
	{
		//Check in each RMC for hit or miss
		min = 0xFFFFFFFFFFFFFFFF;
		for(i=0; i<64; i++)
		{
			if(current->rmc_base[2][i]<=address && address<current->rmc_limit[2][i])
			{
				rmc_hit[2] = 1;
				current->rmc_lru[2][i] = current->total_dtlb_misses;
				current->rmc_hit_base[2] = current->rmc_base[2][i];
				current->rmc_hit_limit[2] = current->rmc_limit[2][i];
			}
			if(current->rmc_lru[2][i]<min)
			{
				rmc_lru[2] = i;
				min = current->rmc_lru[2][i];
			}
		}
		if(rmc_hit[2]==1)
			current->rmc_hits[2]++;
		else
		{
			current->rmc_misses[2]++;
		}
	}
	if(current->rmc_hit_base[3]<=address && address<current->rmc_hit_limit[3])
	{
		rmc_hit[3] = 1;
		current->rmc_buffer_hits[3]++;
	}
	if(rmc_hit[3] == 0)
	{
		//Check in each RMC for hit or miss
		min = 0xFFFFFFFFFFFFFFFF;
		for(i=0; i<128; i++)
		{
			if(current->rmc_base[3][i]<=address && address<current->rmc_limit[3][i])
			{
				rmc_hit[3] = 1;
				current->rmc_lru[3][i] = current->total_dtlb_misses;
				current->rmc_hit_base[3] = current->rmc_base[3][i];
				current->rmc_hit_limit[3] = current->rmc_limit[3][i];
			}
			if(current->rmc_lru[3][i]<min)
			{
				rmc_lru[3] = i;
				min = current->rmc_lru[3][i];
			}
		}
		if(rmc_hit[3]==1)
			current->rmc_hits[3]++;
		else
		{
			current->rmc_misses[3]++;
		}
	}
	if(current->rmc_hit_base[4]<=address && address<current->rmc_hit_limit[4])
	{
		rmc_hit[4] = 1;
		current->rmc_buffer_hits[4]++;
	}
	if(rmc_hit[4] == 0)
	{
		//Check in each RMC for hit or miss
		min = 0xFFFFFFFFFFFFFFFF;
		for(i=0; i<2; i++)
		{
			if(current->rmc_base[4][i]<=address && address<current->rmc_limit[4][i])
			{
				rmc_hit[4] = 1;
				current->rmc_lru[4][i] = current->total_dtlb_misses;
				current->rmc_hit_base[4] = current->rmc_base[4][i];
				current->rmc_hit_limit[4] = current->rmc_limit[4][i];
			}
			if(current->rmc_lru[4][i]<min)
			{
				rmc_lru[4] = i;
				min = current->rmc_lru[4][i];
			}
		}
		if(rmc_hit[4]==1)
			current->rmc_hits[4]++;
		else
		{
			current->rmc_misses[4]++;
		}
	}
	if(current->rmc_hit_base[5]<=address && address<current->rmc_hit_limit[5])
	{
		rmc_hit[5] = 1;
		current->rmc_buffer_hits[5]++;
	}
	if(rmc_hit[5] == 0)
	{
		//Check in each RMC for hit or miss
		min = 0xFFFFFFFFFFFFFFFF;
		for(i=0; i<4; i++)
		{
			if(current->rmc_base[5][i]<=address && address<current->rmc_limit[5][i])
			{
				rmc_hit[5] = 1;
				current->rmc_lru[5][i] = current->total_dtlb_misses;
				current->rmc_hit_base[5] = current->rmc_base[5][i];
				current->rmc_hit_limit[5] = current->rmc_limit[5][i];
			}
			if(current->rmc_lru[5][i]<min)
			{
				rmc_lru[5] = i;
				min = current->rmc_lru[5][i];
			}
		}
		if(rmc_hit[5]==1)
			current->rmc_hits[5]++;
		else
		{
			current->rmc_misses[5]++;
		}
	}
	if(current->rmc_hit_base[6]<=address && address<current->rmc_hit_limit[6])
	{
		rmc_hit[6] = 1;
		current->rmc_buffer_hits[6]++;
	}
	if(rmc_hit[6] == 0)
	{
		//Check in each RMC for hit or miss
		min = 0xFFFFFFFFFFFFFFFF;
		for(i=0; i<8; i++)
		{
			if(current->rmc_base[6][i]<=address && address<current->rmc_limit[6][i])
			{
				rmc_hit[6] = 1;
				current->rmc_lru[6][i] = current->total_dtlb_misses;
				current->rmc_hit_base[6] = current->rmc_base[6][i];
				current->rmc_hit_limit[6] = current->rmc_limit[6][i];
			}
			if(current->rmc_lru[6][i]<min)
			{
				rmc_lru[6] = i;
				min = current->rmc_lru[6][i];
			}
		}
		if(rmc_hit[6]==1)
			current->rmc_hits[6]++;
		else
		{
			current->rmc_misses[6]++;
		}
	}
*/
	for(i=0; i<7; i++)
	{
		if(current->rmc_hit_base[i]<=address && address<current->rmc_hit_limit[i])
		{
			rmc_hit[i] = 1;
			current->rmc_buffer_hits[i]++;
		}
		if(rmc_hit[i] == 0)
		{
			//Check in each RMC for hit or miss
			min = 0xFFFFFFFFFFFFFFFF;
			for(j=0; j<(2<<i); j++)
			{
				if(current->rmc_base[i][j]<=address && address<current->rmc_limit[i][j])
				{
					rmc_hit[i] = 1;
					current->rmc_lru[i][j] = current->total_dtlb_misses;
					current->rmc_hit_base[i] = current->rmc_base[i][j];
					current->rmc_hit_limit[i] = current->rmc_limit[i][j];
				}
				if(current->rmc_lru[i][j]<min)
				{
					rmc_lru[i] = j;
					min = current->rmc_lru[i][j];
				}
			}
			if(rmc_hit[i]==1)
				current->rmc_hits[i]++;
			else
			{
				current->rmc_misses[i]++;
			}
		}
	}

	//Introduce new entries in to the RMC
	for(i=0; i<7; i++)
	{
		if(rmc_hit[i]==0 && rmt_hit==1)
		{
			current->rmc_base[i][(rmc_lru[i])] = found_base;
			current->rmc_limit[i][(rmc_lru[i])] = found_limit;
			current->rmc_lru[i][(rmc_lru[i])] = current->total_dtlb_misses;
			current->rmc_hit_base[i] = found_base;
			current->rmc_hit_limit[i] = found_limit;
		}
	}
	return 1;
}
