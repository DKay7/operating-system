#include <inc/types.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/uefi.h>
#include <kern/timer.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

static void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is requrired?)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */
    typedef struct {
        ACPISDTHeader h;
        uint64_t PointerToOtherSDT[];
    } __attribute__((packed)) XSDT;
    static RSDP *rsdp;
    ACPISDTHeader *root;
    size_t entry_size;
    size_t entries;

    bool checksum_ok(const void *table, size_t len) {
        const uint8_t *bytes = table;
        uint8_t sum = 0;

        for (size_t i = 0; i < len; i++)
            sum += bytes[i];

        return sum == 0;
    }

    ACPISDTHeader *map_sdt(physaddr_t pa) {
        ACPISDTHeader *table = mmio_map_region(pa, sizeof(*table));
        table = mmio_remap_last_region(pa, table, sizeof(*table), table->Length);

        if (!checksum_ok(table, table->Length))
            panic("ACPI table %.4s checksum mismatch", table->Signature);

        return table;
    }

    if (rsdp == NULL) {
        physaddr_t pa = uefi_lp->ACPIRoot;
        size_t len;

        rsdp = mmio_map_region(pa, sizeof(*rsdp));
        len = rsdp->Revision >= 2 && rsdp->Length ? rsdp->Length : 20;
        rsdp = mmio_remap_last_region(pa, rsdp, sizeof(*rsdp), len);

        if (memcmp(rsdp->Signature, "RSD PTR ", sizeof(rsdp->Signature)) != 0)
            panic("Invalid ACPI RSDP signature");
        if (!checksum_ok(rsdp, 20))
            panic("RSDP checksum mismatch");
        if (rsdp->Revision >= 2 && !checksum_ok(rsdp, rsdp->Length))
            panic("XSDP checksum mismatch");
    }

    if (rsdp->Revision >= 2 && rsdp->XsdtAddress) {
        root = map_sdt(rsdp->XsdtAddress);
        entry_size = sizeof(uint64_t);
    } else {
        root = map_sdt(rsdp->RsdtAddress);
        entry_size = sizeof(uint32_t);
    }

    entries = (root->Length - sizeof(*root)) / entry_size;
    for (size_t i = 0; i < entries; i++) {
        physaddr_t pa;
        ACPISDTHeader *table;

        if (entry_size == sizeof(uint64_t))
            pa = ((XSDT *)root)->PointerToOtherSDT[i];
        else
            pa = ((RSDT *)root)->PointerToOtherSDT[i];

        table = map_sdt(pa);
        if (memcmp(table->Signature, sign, 4) == 0)
            return table;
    }

    return NULL;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    static FADT *fadt;

    if (fadt == NULL) {
        fadt = acpi_find_table("FACP");
        if (fadt == NULL)
            panic("FADT table not found");
    }

    return fadt;
}

/* Obtain and map HPET ACPI table address. */
HPET *
get_hpet(void) {
    static HPET *hpet;

    if (hpet == NULL) {
        hpet = acpi_find_table("HPET");
        if (hpet == NULL)
            panic("HPET table not found");
    }

    return hpet;
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    HPET *hpet = get_hpet();
    assert(hpet != NULL);
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);
        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        // cprintf("hpetFemto = %llu\n", hpetFemto);
        hpetFreq = (1 * Peta) / hpetFemto;
        // cprintf("HPET: Frequency = %d.%03dMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega));
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */
void
hpet_enable_interrupts_tim0(void) {
    uint64_t conf;

    hpet_init();
    conf = hpetReg->TIM0_CONF;
    if (!(conf & HPET_TN_PER_INT_CAP))
        panic("HPET timer 0 does not support periodic mode");

    hpetReg->GEN_CONF &= ~HPET_ENABLE_CNF;
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;
    hpetReg->MAIN_CNT = 0;
    conf |= HPET_TN_TYPE_CNF | HPET_TN_INT_ENB_CNF | HPET_TN_VAL_SET_CNF;
    hpetReg->TIM0_CONF = conf;
    hpetReg->TIM0_COMP = hpetFreq / 2;
    hpetReg->TIM0_COMP = hpetFreq / 2;
    pic_irq_unmask(IRQ_TIMER);
    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
}

void
hpet_enable_interrupts_tim1(void) {
    uint64_t conf;

    hpet_init();
    conf = hpetReg->TIM1_CONF;
    if (!(conf & HPET_TN_PER_INT_CAP))
        panic("HPET timer 1 does not support periodic mode");

    hpetReg->GEN_CONF &= ~HPET_ENABLE_CNF;
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;
    hpetReg->MAIN_CNT = 0;
    conf |= HPET_TN_TYPE_CNF | HPET_TN_INT_ENB_CNF | HPET_TN_VAL_SET_CNF;
    hpetReg->TIM1_CONF = conf;
    hpetReg->TIM1_COMP = (hpetFreq * 3) / 2;
    hpetReg->TIM1_COMP = (hpetFreq * 3) / 2;
    pic_irq_unmask(IRQ_CLOCK);
    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * about pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq;

    if (!cpu_freq) {
        uint64_t start_hpet, end_hpet;
        uint64_t start_tsc, end_tsc;
        uint64_t delta_hpet;

        hpet_init();

        start_hpet = hpet_get_main_cnt();
        start_tsc = read_tsc();
        do {
            asm volatile("pause");
            end_hpet = hpet_get_main_cnt();
        } while (end_hpet - start_hpet < hpetFreq / 10);
        end_tsc = read_tsc();

        delta_hpet = end_hpet - start_hpet;
        cpu_freq = (end_tsc - start_tsc) * hpetFreq / delta_hpet;
    }

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq;

    if (!cpu_freq) {
        FADT *fadt = get_fadt();
        uint64_t mask = (fadt->Flags & (1 << 8)) ? 0xFFFFFFFFULL : 0xFFFFFFULL;
        uint64_t start_pm, end_pm;
        uint64_t start_tsc, end_tsc;
        uint64_t delta_pm;

        start_pm = pmtimer_get_timeval() & mask;
        start_tsc = read_tsc();
        do {
            asm volatile("pause");
            end_pm = pmtimer_get_timeval() & mask;
            delta_pm = (end_pm - start_pm) & mask;
        } while (delta_pm < PM_FREQ / 10);
        end_tsc = read_tsc();

        cpu_freq = (end_tsc - start_tsc) * PM_FREQ / delta_pm;
    }

    return cpu_freq;
}
