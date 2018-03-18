#include <kernel/io.h>
#include <kernel/pci.h>
#include <stdio.h>

#define PCI_CFG_OUT_PORT    (0xcf8)
#define PCI_CFG_IN_PORT     (0xcfc)
#define PCI_NO_DEVICE       (0xffff)

uint16_t pci_cfg_readw(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
    uint32_t address =
        (1ul << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)func << 8) |
        ((uint32_t)offset << 2);

    outl(PCI_CFG_OUT_PORT, address);
    return (inl(PCI_CFG_IN_PORT) >> ((offset & 2) * 8)) & 0xffff;
}

void pci_init()
{
    for (int bus=0; bus<256; bus++) {
        for (int dev=0; dev<32; dev++) {
            uint16_t vendor_id = pci_cfg_readw(bus, dev, 0, 0);
            if (vendor_id != PCI_NO_DEVICE) {
                uint16_t device_id = pci_cfg_readw(bus, dev, 0, 2);
                printf("Found PCI device: { vendor_id: %u, device_id: %u }\n",
                    vendor_id, device_id);
            }
        }
    }
}
