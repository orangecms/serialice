/*
 * QEMU PowerPC E500 embedded processors pci controller emulation
 *
 * Copyright (C) 2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Yu Liu,     <yu.liu@freescale.com>
 *
 * This file is derived from hw/ppc4xx_pci.c,
 * the copyright for that material belongs to the original owners.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of  the GNU General  Public License as published by
 * the Free Software Foundation;  either version 2 of the  License, or
 * (at your option) any later version.
 */

#include "hw.h"
#include "pci.h"
#include "pci_host.h"
#include "bswap.h"

#ifdef DEBUG_PCI
#define pci_debug(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#else
#define pci_debug(fmt, ...)
#endif

#define PCIE500_CFGADDR       0x0
#define PCIE500_CFGDATA       0x4
#define PCIE500_REG_BASE      0xC00
#define PCIE500_ALL_SIZE      0x1000
#define PCIE500_REG_SIZE      (PCIE500_ALL_SIZE - PCIE500_REG_BASE)

#define PPCE500_PCI_CONFIG_ADDR         0x0
#define PPCE500_PCI_CONFIG_DATA         0x4
#define PPCE500_PCI_INTACK              0x8

#define PPCE500_PCI_OW1                 (0xC20 - PCIE500_REG_BASE)
#define PPCE500_PCI_OW2                 (0xC40 - PCIE500_REG_BASE)
#define PPCE500_PCI_OW3                 (0xC60 - PCIE500_REG_BASE)
#define PPCE500_PCI_OW4                 (0xC80 - PCIE500_REG_BASE)
#define PPCE500_PCI_IW3                 (0xDA0 - PCIE500_REG_BASE)
#define PPCE500_PCI_IW2                 (0xDC0 - PCIE500_REG_BASE)
#define PPCE500_PCI_IW1                 (0xDE0 - PCIE500_REG_BASE)

#define PPCE500_PCI_GASKET_TIMR         (0xE20 - PCIE500_REG_BASE)

#define PCI_POTAR               0x0
#define PCI_POTEAR              0x4
#define PCI_POWBAR              0x8
#define PCI_POWAR               0x10

#define PCI_PITAR               0x0
#define PCI_PIWBAR              0x8
#define PCI_PIWBEAR             0xC
#define PCI_PIWAR               0x10

#define PPCE500_PCI_NR_POBS     5
#define PPCE500_PCI_NR_PIBS     3

struct  pci_outbound {
    uint32_t potar;
    uint32_t potear;
    uint32_t powbar;
    uint32_t powar;
};

struct pci_inbound {
    uint32_t pitar;
    uint32_t piwbar;
    uint32_t piwbear;
    uint32_t piwar;
};

struct PPCE500PCIState {
    PCIHostState pci_state;
    struct pci_outbound pob[PPCE500_PCI_NR_POBS];
    struct pci_inbound pib[PPCE500_PCI_NR_PIBS];
    uint32_t gasket_time;
    qemu_irq irq[4];
    /* mmio maps */
    int cfgaddr;
    int cfgdata;
    int reg;
};

typedef struct PPCE500PCIState PPCE500PCIState;

static uint32_t pci_reg_read4(void *opaque, target_phys_addr_t addr)
{
    PPCE500PCIState *pci = opaque;
    unsigned long win;
    uint32_t value = 0;

    win = addr & 0xfe0;

    switch (win) {
    case PPCE500_PCI_OW1:
    case PPCE500_PCI_OW2:
    case PPCE500_PCI_OW3:
    case PPCE500_PCI_OW4:
        switch (addr & 0xC) {
        case PCI_POTAR: value = pci->pob[(addr >> 5) & 0x7].potar; break;
        case PCI_POTEAR: value = pci->pob[(addr >> 5) & 0x7].potear; break;
        case PCI_POWBAR: value = pci->pob[(addr >> 5) & 0x7].powbar; break;
        case PCI_POWAR: value = pci->pob[(addr >> 5) & 0x7].powar; break;
        default: break;
        }
        break;

    case PPCE500_PCI_IW3:
    case PPCE500_PCI_IW2:
    case PPCE500_PCI_IW1:
        switch (addr & 0xC) {
        case PCI_PITAR: value = pci->pib[(addr >> 5) & 0x3].pitar; break;
        case PCI_PIWBAR: value = pci->pib[(addr >> 5) & 0x3].piwbar; break;
        case PCI_PIWBEAR: value = pci->pib[(addr >> 5) & 0x3].piwbear; break;
        case PCI_PIWAR: value = pci->pib[(addr >> 5) & 0x3].piwar; break;
        default: break;
        };
        break;

    case PPCE500_PCI_GASKET_TIMR:
        value = pci->gasket_time;
        break;

    default:
        break;
    }

    pci_debug("%s: win:%lx(addr:" TARGET_FMT_plx ") -> value:%x\n", __func__,
              win, addr, value);
    return value;
}

static CPUReadMemoryFunc * const e500_pci_reg_read[] = {
    &pci_reg_read4,
    &pci_reg_read4,
    &pci_reg_read4,
};

static void pci_reg_write4(void *opaque, target_phys_addr_t addr,
                               uint32_t value)
{
    PPCE500PCIState *pci = opaque;
    unsigned long win;

    win = addr & 0xfe0;

    pci_debug("%s: value:%x -> win:%lx(addr:" TARGET_FMT_plx ")\n",
              __func__, value, win, addr);

    switch (win) {
    case PPCE500_PCI_OW1:
    case PPCE500_PCI_OW2:
    case PPCE500_PCI_OW3:
    case PPCE500_PCI_OW4:
        switch (addr & 0xC) {
        case PCI_POTAR: pci->pob[(addr >> 5) & 0x7].potar = value; break;
        case PCI_POTEAR: pci->pob[(addr >> 5) & 0x7].potear = value; break;
        case PCI_POWBAR: pci->pob[(addr >> 5) & 0x7].powbar = value; break;
        case PCI_POWAR: pci->pob[(addr >> 5) & 0x7].powar = value; break;
        default: break;
        };
        break;

    case PPCE500_PCI_IW3:
    case PPCE500_PCI_IW2:
    case PPCE500_PCI_IW1:
        switch (addr & 0xC) {
        case PCI_PITAR: pci->pib[(addr >> 5) & 0x3].pitar = value; break;
        case PCI_PIWBAR: pci->pib[(addr >> 5) & 0x3].piwbar = value; break;
        case PCI_PIWBEAR: pci->pib[(addr >> 5) & 0x3].piwbear = value; break;
        case PCI_PIWAR: pci->pib[(addr >> 5) & 0x3].piwar = value; break;
        default: break;
        };
        break;

    case PPCE500_PCI_GASKET_TIMR:
        pci->gasket_time = value;
        break;

    default:
        break;
    };
}

static CPUWriteMemoryFunc * const e500_pci_reg_write[] = {
    &pci_reg_write4,
    &pci_reg_write4,
    &pci_reg_write4,
};

static int mpc85xx_pci_map_irq(PCIDevice *pci_dev, int irq_num)
{
    int devno = pci_dev->devfn >> 3, ret = 0;

    switch (devno) {
        /* Two PCI slot */
        case 0x11:
        case 0x12:
            ret = (irq_num + devno - 0x10) % 4;
            break;
        default:
            printf("Error:%s:unknown dev number\n", __func__);
    }

    pci_debug("%s: devfn %x irq %d -> %d  devno:%x\n", __func__,
           pci_dev->devfn, irq_num, ret, devno);

    return ret;
}

static void mpc85xx_pci_set_irq(void *opaque, int irq_num, int level)
{
    qemu_irq *pic = opaque;

    pci_debug("%s: PCI irq %d, level:%d\n", __func__, irq_num, level);

    qemu_set_irq(pic[irq_num], level);
}

static const VMStateDescription vmstate_pci_outbound = {
    .name = "pci_outbound",
    .version_id = 0,
    .minimum_version_id = 0,
    .minimum_version_id_old = 0,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(potar, struct pci_outbound),
        VMSTATE_UINT32(potear, struct pci_outbound),
        VMSTATE_UINT32(powbar, struct pci_outbound),
        VMSTATE_UINT32(powar, struct pci_outbound),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_pci_inbound = {
    .name = "pci_inbound",
    .version_id = 0,
    .minimum_version_id = 0,
    .minimum_version_id_old = 0,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32(pitar, struct pci_inbound),
        VMSTATE_UINT32(piwbar, struct pci_inbound),
        VMSTATE_UINT32(piwbear, struct pci_inbound),
        VMSTATE_UINT32(piwar, struct pci_inbound),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_ppce500_pci = {
    .name = "ppce500_pci",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_STRUCT_ARRAY(pob, PPCE500PCIState, PPCE500_PCI_NR_POBS, 1,
                             vmstate_pci_outbound, struct pci_outbound),
        VMSTATE_STRUCT_ARRAY(pib, PPCE500PCIState, PPCE500_PCI_NR_PIBS, 1,
                             vmstate_pci_outbound, struct pci_inbound),
        VMSTATE_UINT32(gasket_time, PPCE500PCIState),
        VMSTATE_END_OF_LIST()
    }
};

static void e500_pci_map(SysBusDevice *dev, target_phys_addr_t base)
{
    PCIHostState *h = FROM_SYSBUS(PCIHostState, sysbus_from_qdev(dev));
    PPCE500PCIState *s = DO_UPCAST(PPCE500PCIState, pci_state, h);

    cpu_register_physical_memory(base + PCIE500_CFGADDR, 4, s->cfgaddr);
    cpu_register_physical_memory(base + PCIE500_CFGDATA, 4, s->cfgdata);
    cpu_register_physical_memory(base + PCIE500_REG_BASE, PCIE500_REG_SIZE,
                                 s->reg);
}

static int e500_pcihost_initfn(SysBusDevice *dev)
{
    PCIHostState *h;
    PPCE500PCIState *s;
    PCIBus *b;
    int i;

    h = FROM_SYSBUS(PCIHostState, sysbus_from_qdev(dev));
    s = DO_UPCAST(PPCE500PCIState, pci_state, h);

    for (i = 0; i < ARRAY_SIZE(s->irq); i++) {
        sysbus_init_irq(dev, &s->irq[i]);
    }

    b = pci_register_bus(&s->pci_state.busdev.qdev, NULL, mpc85xx_pci_set_irq,
                         mpc85xx_pci_map_irq, s->irq, PCI_DEVFN(0x11, 0), 4);
    s->pci_state.bus = b;

    pci_create_simple(b, 0, "e500-host-bridge");

    s->cfgaddr = pci_host_conf_register_mmio(&s->pci_state, DEVICE_BIG_ENDIAN);
    s->cfgdata = pci_host_data_register_mmio(&s->pci_state,
                                             DEVICE_LITTLE_ENDIAN);
    s->reg = cpu_register_io_memory(e500_pci_reg_read, e500_pci_reg_write, s,
                                    DEVICE_BIG_ENDIAN);
    sysbus_init_mmio_cb(dev, PCIE500_ALL_SIZE, e500_pci_map);

    return 0;
}

static PCIDeviceInfo e500_host_bridge_info = {
    .qdev.name    = "e500-host-bridge",
    .qdev.desc    = "Host bridge",
    .qdev.size    = sizeof(PCIDevice),
    .vendor_id    = PCI_VENDOR_ID_FREESCALE,
    .device_id    = PCI_DEVICE_ID_MPC8533E,
    .class_id     = PCI_CLASS_PROCESSOR_POWERPC,
};

static SysBusDeviceInfo e500_pcihost_info = {
    .init         = e500_pcihost_initfn,
    .qdev.name    = "e500-pcihost",
    .qdev.size    = sizeof(PPCE500PCIState),
    .qdev.vmsd    = &vmstate_ppce500_pci,
};

static void e500_pci_register(void)
{
    sysbus_register_withprop(&e500_pcihost_info);
    pci_qdev_register(&e500_host_bridge_info);
}
device_init(e500_pci_register);
