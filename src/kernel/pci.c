#include <kernel/io.h>
#include <kernel/pci.h>
#include <stdio.h>

#define PCI_CFG_OUT_PORT    (0xcf8)
#define PCI_CFG_IN_PORT     (0xcfc)
#define PCI_NO_DEVICE       (0xffff)

static uint16_t pci_cfg_readw(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
    uint32_t address =
        (1ul << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)func << 8) |
        ((uint32_t)offset << 2);

    outl(PCI_CFG_OUT_PORT, address);
    return (inl(PCI_CFG_IN_PORT) >> (((uint32_t)offset & 2) * 8)) & 0xffff;
}

static void pci_check_func(uint8_t bus, uint8_t device, uint8_t func)
{
    uint16_t vendor_id = pci_cfg_readw(bus, device, 0, 0); // dupe
    uint16_t device_id = pci_cfg_readw(bus, device, 0, 2); // dupe

    uint16_t prog_rev = pci_cfg_readw(bus, device, 0, 8);
    uint8_t prog_if = (prog_rev >> 8) & 0xff;
    uint8_t rev = prog_rev & 0xff;

    uint16_t class_subclass = pci_cfg_readw(bus, device, 0, 10);
    uint8_t class_code = (class_subclass >> 8) & 0xff;
    uint8_t subclass = class_subclass & 0xff;

    uint16_t bist_header = pci_cfg_readw(bus, device, 0, 14);
    uint8_t bist = (bist_header >> 8) & 0xff;
    uint8_t header_type = bist_header & 0xff;

    printf("Found PCI device: { "
        "vendor_id: %u, "
        "device_id: %u, "
        "class: %u, "
        "subclass: %u "
        " }\n",
        vendor_id, device_id, class_code, subclass);
    if ((header_type & 0x80) != 0) {
        for (int i=0; i<8; i++) {
            pci_check_func(bus, device, i);
        }
    }
}

static void pci_check_slot(uint8_t bus, uint8_t device)
{
    uint16_t vendor_id = pci_cfg_readw(bus, device, 0, 0);
    if (vendor_id == PCI_NO_DEVICE) {
        goto done;
    }

    uint16_t device_id = pci_cfg_readw(bus, device, 0, 2);

    pci_check_func(bus, device, 0);

done:
    return;
}

void pci_init()
{
    for (int bus=0; bus<256; bus++) {
        for (int dev=0; dev<32; dev++) {
            pci_check_slot(bus, dev);
        }
    }
}
