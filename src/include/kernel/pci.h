#ifndef __ARGIR__PCI_H
#define __ARGIR__PCI_H

#include <stddef.h>
#include <stdint.h>

#define PCI_BAR_COUNT_MAX               (6)
struct pci_descriptor {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint32_t bar[PCI_BAR_COUNT_MAX];
};

#define PCI_DEVICE_COUNT_MAX            (8)
struct pci {
    size_t dev_count;
    struct pci_descriptor dev[PCI_DEVICE_COUNT_MAX];
};

void pci_init(struct pci *pci);

#endif /* __ARGIR__PCI_H */
