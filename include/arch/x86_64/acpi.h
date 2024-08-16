#pragma once
#include <types.h>

#define ACPI_GAS_MEM 0x00
#define ACPI_GAS_IO 0x01

struct acpi_gas {
    u8 AddressSpace;
    u8 BitWidth;
    u8 BitOffset;
    u8 AccessSize;
    u64 Address;
} __attribute__((packed));

struct acpi_rsdp {
    u8 Signature[8];
    u8 Checksum;
    u8 OEMID[6];
    u8 Revision;
    u32 RsdtAddress;
} __attribute__((packed));

struct acpi_rsdp2 {
    struct acpi_rsdp FirstPart;
    u32 Length;
    u64 XsdtAddress;
    u8 ExtendedChecksum;
    u8 Reserved[3];
} __attribute__((packed));

struct acpi_sdt_hdr {
    char Signature[4];
    u32 Length;
    u8 Revision;
    u8 Checksum;
    char OEMID[6];
    char OEMTableID[8];
    u32 OEMRevision;
    u32 CreatorID;
    u32 CreatorRevision;
} __attribute__((packed));

struct acpi_rsdt {
    struct acpi_sdt_hdr h;
    u32 PointerToOtherSDT[];
} __attribute__((packed));

struct acpi_xsdt {
    struct acpi_sdt_hdr h;
    u64 PointerToOtherSDT[];
} __attribute__((packed));

struct acpi_fadt {
    struct acpi_sdt_hdr h;
    u32 FIRMWARE_CTRL;
    u32 DSDT;
    u8 Reserved;
    u8 Preferred_PM_Profile;
    u16 SCI_INT;
    u32 SMI_CMD;
    u8 ACPI_ENABLE;
    u8 ACPI_DISABLE;
    u8 S4BIOS_REQ;
    u8 PSTATE_CNT;
    u32 PM1a_EVT_BLK;
    u32 PM1b_EVT_BLK;
    u32 PM1a_CNT_BLK;
    u32 PM1b_CNT_BLK;
    u32 PM2_CNT_BLK;
    u32 PM_TMR_BLK;
    u32 GPE0_BLK;
    u32 GPE1_BLK;
    u8 PM1_EVT_LEN;
    u8 PM1_CNT_LEN;
    u8 PM2_CNT_LEN;
    u8 PM_TMR_LEN;
    u8 GPE0_BLK_LEN;
    u8 GPE1_BLK_LEN;
    u8 GPE1_BASE;
    u8 CST_CNT;
    u16 P_LVL2_LAT;
    u16 P_LVL3_LAT;
    u16 FLUSH_SIZE;
    u16 FLUSH_STRIDE;
    u8 DUTY_OFFSET;
    u8 DUTY_WIDTH;
    u8 DAY_ALRM;
    u8 MON_ALRM;
    u8 CENTURY;
    u16 IAPC_BOOT_ARCH;
    u8 Reserved2;
    u32 Flags;
    struct acpi_gas RESET_REG;
    u8 RESET_VALUE;
    u16 ARM_BOOT_ARCH;
    u8 FADT_Minor_Version;
    u64 X_FIRMWARE_CTRL;
    u64 X_DSDT;
    struct acpi_gas X_PM1a_EVT_BLK;
    struct acpi_gas X_PM1b_EVT_BLK;
    struct acpi_gas X_PM1a_CNT_BLK;
    struct acpi_gas X_PM1b_CNT_BLK;
    struct acpi_gas X_PM2_CNT_BLK;
    struct acpi_gas X_PM_TMR_BLK;
    struct acpi_gas X_GPE0_BLK;
    struct acpi_gas X_GPE1_BLK;
    struct acpi_gas SLEEP_CONTROL_REG;
    struct acpi_gas SLEEP_STATUS_REG;
    u64 HVI;
} __attribute__((packed));

void acpi_init(uptr rsdp);
struct acpi_sdt_hdr *acpi_find_sdt(const char *sig);
struct acpi_fadt *acpi_get_fadt();