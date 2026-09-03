/*
 *
 * project를 위한 가상 가속기 VNPU 코드
 * 단순한 dot8 product 계산만을 진행
 *
 *
 */


#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qemu/module.h"
#include "qapi/visitor.h"
#include <string.h>
#include <stdint.h>


#define TYPE_PCI_VNPU_DEVICE "vnpu"
#define REVISION VNPU_REVISION_A// revision a is 1, b is 2
#define BIT(n) (1 << (n))

#define VEC_LEN_A 0x08
// Revison
#define VNPU_REVISION_A 0x1
#define VNPU_REVISION_B 0x2

// CAPABILITIES
#define VNPU_CAP_DOT_I8 BIT(0) // A, B
#define VNPU_CAP_VECTOR_LEN_8 BIT(1) // A
#define VNPU_CAP_VECTOR_LEN_16 BIT(2) // B
#define VNPU_CAP_JOB_ID BIT(2) // A, B
#define VNPU_CAP_FAULT_INJECTION BIT(3) // A, B
#define VNPU_CAP_COMPLETION_IRQ BIT(4) // A, B
#define VNPU_CAP_ERROR_IRQ BIT(5) // A, B

#define VNPU_CAP_A \
	VNPU_CAP_DOT_I8 | VNPU_CAP_VECTOR_LEN_8 | VNPU_CAP_JOB_ID | VNPU_CAP_FAULT_INJECTION | VNPU_CAP_COMPLETION_IRQ | VNPU_CAP_ERROR_IRQ
#define VNPU_CAP_B \
	VNPU_CAP_DOT_I8 | VNPU_CAP_VECTOR_LEN_16 | VNPU_CAP_JOB_ID | VNPU_CAP_FAULT_INJECTION | VNPU_CAP_COMPLETION_IRQ | VNPU_CAP_ERROR_IRQ

#define CONTROL_START BIT(0)
#define CONTROL_RESET BIT(1)

#define STATUS_IDLE 0x0
#define STATUS_BUSY 0x1
#define STATUS_DONE 0x2
#define STATUS_ERROR 0x3

#define IRQ_COMPLETION BIT(0)
#define IRQ_ERROR BIT(1)

#define VNPU_ERR_NONE 0x0
#define VNPU_ERR_INVALID_LENGTH 0x1
#define VNPU_ERR_BUSY 0x2
#define VNPU_ERR_FORCED 0x3
#define VNPU_ERR_UNSUPPORTED_REVISION 0x4
#define VNPU_ERR_INTERNAL 0x5

#define FAULT_IRQ_DROP BIT(0)
#define FAULT_STUCK_BUSY BIT(1)
#define FAULT_CORRUPT_RESULT BIT(2)
#define FAULT_FORCE_ERROR BIT(3)

typedef struct VnpuState VnpuState;
DECLARE_INSTANCE_CHECKER(VnpuState, VNPU, TYPE_PCI_VNPU_DEVICE)

struct VnpuRevsionOps{
	uint32_t revision;
	uint32_t capabilities;
	uint32_t vec_len;
};

struct VnpuState {
	PCIDevice pdev;
	MemoryRegion mmio;
	uint32_t status;
	uint32_t err_code;

	uint32_t input_a[4];  
	uint32_t input_b[4];
	int32_t result;

	uint32_t control;
	uint32_t job_ib; // new function
	
	uint32_t irq_status;
	uint32_t irq_enable;
	uint32_t fault_control;

	QEMUTimer dot_timer;

	const struct VnpuRevsionOps *ops;
};

static uint64_t vnpu_mmio_read(void *opaque, hwaddr addr, unsigned size);
static void vnpu_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size);
static void vnpu_update_irq(VnpuState *vnpu);
static int32_t unpack_i8(uint32_t input, unsigned int shift);
static int32_t vnpu_dot_product(void *opaque);
static void vnpu_dot_timer(void *opaque);
static void vnpu_register_init(VnpuState *vnpu);
static void pci_vnpu_realize(PCIDevice *pdev, Error **errp);
static void pci_vnpu_uninit(PCIDevice *pdev);
static void vnpu_class_init(ObjectClass *class, const void *data);

static uint64_t vnpu_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    VnpuState *vnpu = opaque;
    uint64_t val = 0ULL;
	
	if(size != 4){
		return val;
	}

	switch (addr) {
	case 0x000:
		val = 0x564E5055;
		break;
	case 0x004:
		val = vnpu->ops->revision;
		break;
	case 0x008:
		val = vnpu->ops->capabilities;
		break;
	case 0x010:
		val = vnpu->status;
		break;
	case 0x014:
		val = vnpu->irq_status;
		break;
	case 0x018:
		val = vnpu->irq_enable;
		break;
	case 0x01C:
		val = vnpu->err_code;
		break;
	case 0x020:
		val = vnpu->job_ib;
		break;
	case 0x024:
		val = vnpu->ops->vec_len;
		break;
	case 0x100:
		val = vnpu->input_a[0];
		break;
	case 0x104:
		val = vnpu->input_a[1];
		break;
	case 0x108:
		val = vnpu->input_a[2];
		break;
	case 0x10C:
		val = vnpu->input_a[3];
		break;
	case 0x120:
		val = vnpu->input_b[0];
		break;
	case 0x124:
		val = vnpu->input_b[1];
		break;
	case 0x128:
		val = vnpu->input_b[2];
		break;
	case 0x12C:
		val = vnpu->input_b[3];
		break;
	case 0x140:
		val = vnpu->result;
		if(vnpu->status == STATUS_DONE){
			vnpu->status = STATUS_IDLE;
		}
		break;
	case 0x180:
		val = vnpu->fault_control;
		break;
	}
	return val;
}

static void vnpu_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                unsigned size)
{

    VnpuState *vnpu = opaque;

	if(size !=4) {
		return;
	}
	switch (addr) {
		case 0x00C: //CONTROL
			if(val & CONTROL_RESET){
				timer_del(&vnpu->dot_timer);
				vnpu->status = STATUS_IDLE;
				vnpu->irq_status = 0;
				vnpu->err_code = 0;
				vnpu->fault_control = 0;
				vnpu_update_irq(vnpu);
				//stopping
				break;
			}
			if(val & CONTROL_START){
				if(vnpu->status != STATUS_IDLE){
					vnpu->irq_status |= IRQ_ERROR;
					if(vnpu->status == STATUS_BUSY)
						vnpu->err_code = VNPU_ERR_BUSY;
					else if(vnpu->status == STATUS_DONE)
						vnpu->err_code = VNPU_ERR_BUSY;
					vnpu_update_irq(vnpu);
					break;
					// send VNPU_ERR_BUSY
					/* Device is not idle. */
				}
				vnpu->status = STATUS_BUSY;
				vnpu->irq_status = 0;
				vnpu_update_irq(vnpu); // 이녀석이 왜 여기 에 있지? timer_mod 뒤에 있어야하는데? 애초에 timer_mod에 구현이 되어 있는디?
				vnpu->err_code = 0;
				timer_mod(&vnpu->dot_timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL)+100);
				break;
			}
			break; // never reach.
		case 0x014: //clear IRQ_STATUS
			vnpu->irq_status &= ~val;
			vnpu_update_irq(vnpu);
			break;
		case 0x018: //IRQ_ENABLE
			vnpu->irq_enable = val & (IRQ_ERROR | IRQ_COMPLETION);
			vnpu_update_irq(vnpu);
			break;
		case 0x020: //JOB_ID
			vnpu->job_ib = val;
			break;
		case 0x024: //VECTOR_LENGTH
			if(vnpu->ops->revision == VNPU_REVISION_A){ 
				if(val == 8){
					vnpu->ops->vec_len = val;
				}
				else{
					vnpu->err_code = VNPU_ERR_INVALID_LENGTH;
					vnpu->status = STATUS_ERROR;
					vnpu->irq_status = IRQ_ERROR;
					vnpu_update_irq(vnpu);
				}
			}
			else if(vnpu->ops->revision == VNPU_REVISION_B){
				if(val == 8 || val == 16){
					vnpu->ops->vec_len = val;
				}
				else {
					vnpu->err_code = VNPU_ERR_INVALID_LENGTH;
					vnpu->status = STATUS_ERROR;
					vnpu->irq_status = IRQ_ERROR;
					vnpu_update_irq(vnpu);
				}
			}
			break;
		case 0x100: //write input_a
			vnpu->input_a[0] = (uint32_t)val;
			break;
		case 0x104: //write input_a
			vnpu->input_a[1] = (uint32_t)val;
			break;
		case 0x108: //write input_a
			vnpu->input_a[2] = (uint32_t)val;
			break;
		case 0x10C: //write input_a
			vnpu->input_a[3] = (uint32_t)val;
			break;
		case 0x120: //write input_b
			vnpu->input_b[0] = (uint32_t)val;
			break;
		case 0x124: //write input_b
			vnpu->input_b[1] = (uint32_t)val;
			break;
		case 0x128: //write input_b
			vnpu->input_b[2] = (uint32_t)val;
			break;
		case 0x12C: //write input_b
			vnpu->input_b[3] = (uint32_t)val;
			break;
		case 0x180: //write FAULT_CONTROL
			val = val & 0x0F;
			vnpu->fault_control = val & (0U - val);
			break;

	}
}
static void vnpu_update_irq(VnpuState *vnpu){

	bool pending = vnpu->irq_status & vnpu->irq_enable;
	pci_set_irq(&vnpu->pdev , pending);

}

static const MemoryRegionOps vnpu_mmio_ops = {
    .read = vnpu_mmio_read,
    .write = vnpu_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },

};

static int32_t unpack_i8(uint32_t input, unsigned int shift){

	uint32_t x = (input >> (shift*8)) & 0xFFU;
	return (x & 0x80U) ? (int32_t)(x-0x100U) : (int32_t)x;
}

static int32_t vnpu_dot_product(void *opaque){
	
	VnpuState *vnpu = opaque;
	uint32_t *input_a = vnpu->input_a;
	uint32_t *input_b = vnpu->input_b;
	int32_t result = 0;

	switch(vnpu->ops->vec_len){
		case 0x08:
			for(int i=0; i<2 ; i++){
				result += unpack_i8(input_a[i], 0) * unpack_i8(input_b[i], 0);
				result += unpack_i8(input_a[i], 1) * unpack_i8(input_b[i], 1);
				result += unpack_i8(input_a[i], 2) * unpack_i8(input_b[i], 2);
				result += unpack_i8(input_a[i], 3) * unpack_i8(input_b[i], 3);
				vnpu->status = STATUS_DONE;
				vnpu->irq_status = IRQ_COMPLETION;
			}
			break;
		case 0x10:
			for(int i=0; i<4 ; i++){
				result += unpack_i8(input_a[i], 0) * unpack_i8(input_b[i], 0);
				result += unpack_i8(input_a[i], 1) * unpack_i8(input_b[i], 1);
				result += unpack_i8(input_a[i], 2) * unpack_i8(input_b[i], 2);
				result += unpack_i8(input_a[i], 3) * unpack_i8(input_b[i], 3);
				vnpu->status = STATUS_DONE;
				vnpu->irq_status = IRQ_COMPLETION;
			}
			break;
		default:
			// The vector length is invalid if execution reaches here
			vnpu->status = STATUS_ERROR
			vnpu->irq_status = IRQ_ERROR;
			vnpu->err_code = VNPU_ERR_INVALID_LENGTH;
			break;
	}
	return result;
}

static void vnpu_dot_timer(void *opaque){
	
	VnpuState *vnpu = opaque;
	vnpu->result = vnpu_dot_product(vnpu);
	
	if(vnpu->fault_control & FAULT_IRQ_DROP){
		vnpu->irq_status = 0;
	}
	else if(vnpu->fault_control & FAULT_STUCK_BUSY){
		vnpu->status = STATUS_BUSY;
		vnpu->irq_status = 0;
	}
	else if(vnpu->fault_control & FAULT_CORRUPT_RESULT){
		vnpu->result ^= 1; // Deterministically alter result.
	}
	else if(vnpu->fault_control & FAULT_FORCE_ERROR){
		vnpu->status = STATUS_ERROR;
		vnpu->err_code = VNPU_ERR_FORCED;
		vnpu->irq_status = IRQ_ERROR;
	}
	vnpu_update_irq(vnpu);
}

static void vnpu_register_init(VnpuState *vnpu, uint32_t version){

	if(version  == VNPU_REVISION_A){
		vnpu->control = 0;
		vnpu->status = STATUS_IDLE;
		vnpu->irq_status = 0;
		vnpu->irq_enable = 0;
		vnpu->err_code = 0;
		vnpu->result = 0;
		vnpu->fault_control = 0;
		vnpu->job_ib = 0;
		memset(vnpu->input_a, 0, sizeof(vnpu->input_a));
		memset(vnpu->input_b, 0, sizeof(vnpu->input_b));
		vnpu->ops->capabilities = VNPU_CAP_A;
		vnpu->ops->revision = VNPU_REVISION_A;
		vnpu->ops->vec_len = 8;
	}
	else if (version == VNPU_REVISION_B){
		vnpu->control = 0;
		vnpu->status = STATUS_IDLE;
		vnpu->irq_status = 0;
		vnpu->irq_enable = 0;
		vnpu->err_code = 0;
		vnpu->result = 0;
		vnpu->fault_control = 0;
		vnpu->job_ib = 0;
		memset(vnpu->input_a, 0, sizeof(vnpu->input_a));
		memset(vnpu->input_b, 0, sizeof(vnpu->input_b));
		vnpu->ops->capabilities = VNPU_CAP_B;
		vnpu->ops->revision = VNPU_REVISION_B;
		vnpu->ops->vec_len = 16;
	}
}

static void pci_vnpu_realize(PCIDevice *pdev, Error **errp){

	VnpuState *vnpu = VNPU(pdev);
	uint8_t *pci_conf = pdev->config;

	pci_config_set_interrupt_pin(pci_conf, 1);
	
	vnpu_register_init(vnpu, VNPU_REVISION_B);
	timer_init_ms(&vnpu->dot_timer, QEMU_CLOCK_VIRTUAL, vnpu_dot_timer, vnpu);


	memory_region_init_io(&vnpu->mmio, OBJECT(vnpu), &vnpu_mmio_ops, vnpu, "vnpu-mmio", 0x1000);
	pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &vnpu->mmio);
	
}

static void pci_vnpu_uninit(PCIDevice *pdev)
{
    VnpuState *vnpu = VNPU(pdev);
	timer_del(&vnpu->dot_timer);
	
}

static void vnpu_class_init(ObjectClass *class, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = pci_vnpu_realize;
    k->exit = pci_vnpu_uninit;
    k->vendor_id = 0x1B36;
    k->device_id = 0x1000;
    k->revision = 0x10; // is this required?
    k->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo vnpu_types[] = {
	{
		.name = TYPE_PCI_VNPU_DEVICE,
		.parent = TYPE_PCI_DEVICE,
		.instance_size = sizeof(VnpuState),
		.class_init = vnpu_class_init,
		.interfaces = (const InterfaceInfo[]){
			{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
			{},
		}
	}
};

DEFINE_TYPES(vnpu_types)

