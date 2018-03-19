#include <kernel/io.h>
#include <kernel/pci.h>
#include <stdio.h>

#define PCI_CFG_OUT_PORT    (0xcf8)
#define PCI_CFG_IN_PORT     (0xcfc)
#define PCI_NO_DEVICE       (0xffff)

static uint32_t pci_cfg_readl(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
    uint32_t address =
        (1ul << 31) |
        (bus << 16) |
        (device << 11) |
        (func << 8) |
        (offset & 0xfc);

    outl(PCI_CFG_OUT_PORT, address);
    return inl(PCI_CFG_IN_PORT);
}

static void pci_check_slot(uint8_t bus, uint8_t device, uint8_t func)
{
    uint32_t reg0 = pci_cfg_readl(bus, device, func, 0);
    uint16_t device_id = (reg0 >> 16) & 0xffff;
    uint16_t vendor_id = reg0 & 0xffff;
    if (vendor_id == 0xffff) {
        goto done;
    }

    printf("Found PCI device:\n-> vendor: %u, device: %u\n", vendor_id, device_id);

    uint32_t reg8 = pci_cfg_readl(bus, device, func, 8);
    uint16_t reg8_hi = (reg8 >> 16) & 0xffff;
    uint8_t class_code = (reg8_hi >> 8) & 0xff;
    uint8_t subclass = reg8_hi & 0xff;

    printf("-> class: %u, subclass: %u\n", class_code, subclass);

    uint32_t reg12 = pci_cfg_readl(bus, device, func, 12);
    uint16_t reg12_hi = (reg12 >> 16) & 0xff;
    uint8_t bist = (reg12_hi >> 8) & 0xff;
    uint8_t header_type = reg12_hi & 0xff;

    printf("-> header_type: %u\n", header_type);

done:
    return;
}

void pci_init()
{
    printf("\n");
    for (int bus=0; bus<256; bus++) {
        for (int dev=0; dev<32; dev++) {
            pci_check_slot(bus, dev, 0);
        }
    }
    printf("Initialised PCI.\n");
}
