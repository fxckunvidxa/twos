#define PRINT_FMT(x) "acpipmt: " x

#include <arch/x86_64/acpi.h>
#include <arch/x86_64/pio.h>
#include <arch/x86_64/acpipmt.h>
#include <printk.h>
#include <vm.h>

#define FLG_TMR_VAL_EXT (1 << 8)

static bool g_mmio = false;
static u64 g_addr = NULL;
static u32 g_mask;

void acpipmt_init()
{
    struct acpi_fadt *fadt = acpi_find_sdt("FACP");

    if (fadt->PM_TMR_LEN != 4) {
        print("not supported\n");
        return;
    }

    if (fadt->h.Length > offsetof(struct acpi_fadt, X_PM_TMR_BLK) &&
        fadt->X_PM_TMR_BLK.Address) {
        if (fadt->X_PM_TMR_BLK.AddressSpace == ACPI_GAS_MEM) {
            g_mmio = true;
        }
        g_addr = fadt->X_PM_TMR_BLK.Address;
    } else {
        g_addr = fadt->PM_TMR_BLK;
    }

    g_mask = (fadt->Flags & FLG_TMR_VAL_EXT) ? 0xffffffff : 0xffffff;

    print("%s 0x%zx, mask: 0x%08x\n", g_mmio ? "MMIO at" : "IO port", g_addr, g_mask);

    if (g_mmio) {
        g_addr = vm_map_mmio(g_addr, 1);
    }
}

bool acpipmt_is_supported()
{
    return (bool)g_addr;
}

u32 acpipmt_get_mask()
{
    return g_mask;
}

u32 acpipmt_read()
{
    if (!g_addr) {
        return 0;
    }
    if (g_mmio) {
        return *(volatile u32 *)g_addr;
    } else {
        return inl(g_addr);
    }
}