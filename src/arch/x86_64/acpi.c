#define PRINT_FMT(x) "acpi: " x
#include <arch/x86_64/acpi.h>
#include <kstring.h>
#include <printk.h>
#include <vm.h>

static struct acpi_xsdt *g_xsdt;
static struct acpi_fadt *g_fadt;

bool acpi_checksum(struct acpi_sdt_hdr *hdr)
{
    u8 sum = 0;
    for (u32 i = 0; i < hdr->Length; i++) {
        sum += ((u8 *)hdr)[i];
    }
    return sum == 0;
}

struct acpi_sdt_hdr *acpi_find_sdt(const char *sig)
{
    usize ent_num = (g_xsdt->h.Length - sizeof(g_xsdt->h)) / 8;
    for (usize i = 0; i < ent_num; i++) {
        struct acpi_sdt_hdr *hdr = VIRT(g_xsdt->PointerToOtherSDT[i]);
        if (memcmp(hdr->Signature, sig, 4) == 0 && acpi_checksum(hdr)) {
            return hdr;
        }
    }
    return NULL;
}

void acpi_init(uptr rsdp)
{
    struct acpi_rsdp2 *rsdp2 = VIRT(rsdp);
    print("RSDP 0x%016zx 0x%08x (rev. %02x %6.6s)\n", rsdp, rsdp2->Length, rsdp2->FirstPart.Revision,
          rsdp2->FirstPart.OEMID);

    g_xsdt = VIRT(rsdp2->XsdtAddress);
    if (!acpi_checksum(g_xsdt)) {
        panic("bad XSDT checksum");
    }

    print("%.4s 0x%016zx 0x%08x (rev. %02x %6.6s %8.8s %08x %4.4s %08x)\n", g_xsdt->h.Signature, rsdp2->XsdtAddress,
          g_xsdt->h.Length, g_xsdt->h.Revision, g_xsdt->h.OEMID, g_xsdt->h.OEMTableID, g_xsdt->h.OEMRevision,
          &g_xsdt->h.CreatorID, g_xsdt->h.CreatorRevision);

    g_fadt = acpi_find_sdt("FACP");
    if (!g_fadt) {
        panic("FADT not found");
    }

    usize ent_num = (g_xsdt->h.Length - sizeof(g_xsdt->h)) / 8;

    for (usize i = 0; i < ent_num; i++) {
        struct acpi_sdt_hdr *hdr = VIRT(g_xsdt->PointerToOtherSDT[i]);
        print("%.4s 0x%016zx 0x%08x (rev. %02x %6.6s %8.8s %08x %4.4s %08x)\n", hdr->Signature,
              g_xsdt->PointerToOtherSDT[i], hdr->Length, hdr->Revision, hdr->OEMID, hdr->OEMTableID, hdr->OEMRevision,
              &hdr->CreatorID, hdr->CreatorRevision);
    }
}

struct acpi_fadt *acpi_get_fadt()
{
    return g_fadt;
}