#include <memory.h>
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

static void pci_check_slot(struct pci *pci, uint8_t bus, uint8_t device, uint8_t func)
{
    uint32_t reg0 = pci_cfg_readl(bus, device, func, 0);
    uint16_t device_id = (reg0 >> 16) & 0xffff;
    uint16_t vendor_id = reg0 & 0xffff;
    if (vendor_id == 0xffff) {
        goto done;
    }

    uint32_t reg8 = pci_cfg_readl(bus, device, func, 8);
    uint16_t reg8_hi = (reg8 >> 16) & 0xffff;
    uint8_t class_code = (reg8_hi >> 8) & 0xff;
    uint8_t subclass = reg8_hi & 0xff;
    uint16_t reg8_lo = reg8 & 0xffff;
    uint8_t prog_if = (reg8_lo >> 8) & 0xff;
    uint8_t rev_id = reg8_lo & 0xff;

    size_t i = pci->dev_count++;
    pci->dev[i].vendor_id = vendor_id;
    pci->dev[i].device_id = device_id;
    pci->dev[i].class_code = class_code;
    pci->dev[i].subclass = subclass;
    pci->dev[i].prog_if = prog_if;
    pci->dev[i].rev_id = rev_id;

done:
    return;
}

void pci_init(struct pci *pci)
{
    pci->dev_count = 0;
    for (int i=0; i<PCI_DEVICE_COUNT_MAX; i++) {
        struct pci_descriptor *p = pci->dev+i;
        memset(p, 0, sizeof(*p));
        memset(p->bar, 0, PCI_BAR_COUNT_MAX);
    }

    for (int bus=0; bus<256; bus++) {
        for (int dev=0; dev<32; dev++) {
            pci_check_slot(pci, bus, dev, 0);
        }
    }

    printf("Initialised PCI.\n");
}
