#include "qemu/osdep.h"
/* #include "qemu/log.h" */
/* #include "qemu/units.h" */
#include "hw/pci/pci.h"
/* #include "hw/pci/msi.h" */
/* #include "qemu/timer.h" */
#include "qom/object.h"
/* #include "qemu/main-loop.h" */
#include "qemu/module.h"
/* #include "qapi/visitor.h" */

/*
 *	단순한 pci-device
 *	register map은 device_id, revision, capability 만 제공하며
 *	단순히 driver는 read만을 수행할 수 있는 읽기 전용 pci-device이다.
 *
 */


#define TYPE_PCI_SIM_DEVICE "simple-pci"

typedef struct SimState SimState;

DECLARE_INSTANCE_CHECKER(SimState, SIM, TYPE_PCI_SIM_DEVICE)

#define DEVICE_ID 0x564E5055
#define CURRENT_REVISION 0x0 // Revision A == 0 , Revsion B == 1
#define CAPABILITY 0x1

struct SimState {
	PCIDevice pdev;
	MemoryRegion mmio;
	/* bool stopping; */
};


static uint64_t sim_mmio_read(void *opaque, hwaddr addr, unsigned size){
	
	uint64_t val = ~0ULL;

	if(addr > 0x08){
		return val;
	}

	if(size != 4){
		return val;
	}

	switch(addr){
		case 0x00:
			val = 0x564E5055;
			break;
		case 0x04:
			val = CURRENT_REVISION;
			break;
		case 0x08:
			val = CAPABILITY;
			break;
	}

	return val;

}

static const MemoryRegionOps sim_mmio_ops = {
	.read = sim_mmio_read,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.valid = {
		.min_access_size = 4,
		.max_access_size = 4,
	},
	.impl = {
		.min_access_size = 4,
		.max_access_size = 4,
	}
};

static void pci_sim_realize(PCIDevice *pdev, Error **errp){
	SimState *sim = SIM(pdev);
	
	memory_region_init_io(&sim->mmio, OBJECT(sim), &sim_mmio_ops, sim, "sim-mmio", 0x10);
	pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &sim->mmio);
}

/* static void pci_sim_uninit(PCIDevice *pdev){ */
/* 	SimState *sim = SIM(pdev); */
/* 	sim->stopping = true; */
/* } */

static void sim_class_init(ObjectClass *class, const void *data){
	DeviceClass *dc = DEVICE_CLASS(class);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(class);
	
	k->vendor_id = PCI_VENDOR_ID_QEMU;
	k->realize = pci_sim_realize;
	/* k->exit = pci_sim_uninit; */
	k->device_id = 0x5055;
	k->revision = 0x0;
	k->class_id = PCI_CLASS_OTHERS;
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);

}


static const TypeInfo sim_types[] = {
	{
		.name = TYPE_PCI_SIM_DEVICE,
		.parent = TYPE_PCI_DEVICE,
		.class_init = sim_class_init,
		.instance_size = sizeof(SimState),
		.interfaces = (const InterfaceInfo[]){
			{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
			{ },
		},
	}

};

DEFINE_TYPES(sim_types)
