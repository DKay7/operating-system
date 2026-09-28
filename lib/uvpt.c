/* User virtual page table helpers */

#include "inc/error.h"
#include "inc/memlayout.h"
#include "inc/types.h"
#include <inc/lib.h>
#include <inc/mmu.h>
#include <stdint.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

uintptr_t
get_phys_addr(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P))
        return -1;
    if (!(uvpdp[VPDP(va)] & PTE_P))
        return -1;
    if (uvpdp[VPDP(va)] & PTE_PS)
        return PTE_ADDR(uvpdp[VPDP(va)]) + ((uintptr_t)va & ((1ULL << PDP_SHIFT) - 1));
    if (!(uvpd[VPD(va)] & PTE_P))
        return -1;
    if ((uvpd[VPD(va)] & PTE_PS))
        return PTE_ADDR(uvpd[VPD(va)]) + ((uintptr_t)va & ((1ULL << PD_SHIFT) - 1));
    if (!(uvpt[VPT(va)] & PTE_P))
        return -1;
    return PTE_ADDR(uvpt[VPT(va)]) + PAGE_OFFSET(va);
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region.
     * NOTE: Skip over larger pages/page directories for efficiency */
    // LAB 11: Your code here:

    int res = 0;
    if (fun == NULL)
        return -E_INVAL;

    uintptr_t va = 0;

    while (va < MAX_USER_ADDRESS) {

        if (res != 0)
            return res;

        // PML4 entry is not present
        // move va to next pml4
        if (!(uvpml4[VPML4(va)] & PTE_P)) {
            va = ROUNDDOWN(va, 1lu << PML4_SHIFT);
            va = va + (1lu << PML4_SHIFT);
            continue;
        }

        // pdp entry not present
        // move va to next pdp
        if (!(uvpdp[VPDP(va)] & PTE_P)) {
            va = ROUNDDOWN(va, 1 << PDP_SHIFT);
            va = va + (1 << PDP_SHIFT);
            continue;
        }

        // pdp entry is present, but has HUGE_PAGE
        if (uvpdp[VPDP(va)] & PTE_PS) {
            // and it has PTE_SHARE, so its our target
            if (uvpdp[VPDP(va)] & PTE_SHARE)
                res = fun((void *)va, (void *)(va + (1 << PDP_SHIFT)), arg);

            va = ROUNDDOWN(va, 1 << PDP_SHIFT);
            va = va + (1 << PDP_SHIFT);
            continue;
        }

        // pd entry not present
        // move va to next pd
        if (!(uvpd[VPD(va)] & PTE_P)) {
            va = ROUNDDOWN(va, 1 << PD_SHIFT);
            va = va + (1 << PD_SHIFT);
            continue;
        }

        // pd entry is present, but has HUGE_PAGE
        if ((uvpd[VPD(va)] & PTE_PS)) {
            // and it has PTE_SHARE, so its our target
            if (uvpd[VPD(va)] & PTE_SHARE)
                res = fun((void *)va, (void *)(va + (1 << PD_SHIFT)), arg);

            va = ROUNDDOWN(va, 1 << PD_SHIFT);
            va = va + (1 << PD_SHIFT);
            continue;
        }

        // pt entry is present
        if ((uvpt[VPT(va)] & PTE_P) && (uvpt[VPT(va)] & PTE_SHARE)) {
            res = fun((void *)va, (void *)(va + PAGE_SIZE), arg);
        }

        va += PAGE_SIZE;
    }

    return res;
}
