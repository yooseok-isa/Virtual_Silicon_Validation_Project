#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/completion.h>

#include "vnpu-uapi.h"

#define BIT(n) (1<<n)

#define VNPU_VENDOR_ID 0x1B36
#define VNPU_DEVICE_ID 0x1000

#define VNPU_IOCTL_GET_INFO 0x00
#define VNPU_IOCTL_RUN_DOT 0x01
#define VNPU_IOCTL_RESET 0x02
#define VNPU_IOCTL_SET_FAULT 0x03
#define VNPU_IOCTL_GET_STAT 0x04

#define VNPU_REG_DEVICE_ID 0x000
#define VNPU_REG_REVISION 0x004
#define VNPU_REG_CONTROL 0x00C
#define VNPU_REG_STATUS 0x010
#define VNPU_REG_IRQ_STATUS 0x014
#define VNPU_REG_IRQ_ENABLE 0x018
#define VNPU_REG_ERROR_CODE 0x01C
#define VNPU_REG_VECTOR_LENGTH 0x024
#define VNPU_REG_INPUT_A_BASE 0x100
#define VNPU_REG_INPUT_B_BASE 0x120
#define VNPU_REG_RESULT 0x140
#define VNPU_REG_FUALT_CONTROL 0x180

#define VNPU_CONTROL_START BIT(0)
#define VNPU_CONTROL_RESET BIT(1)

#define VNPU_IRQ_COMPLETION BIT(0)
#define VNPU_IRQ_ERROR BIT(1)
#define VNPU_IRQ_ALL (VNPU_IRQ_COMPLETION | VNPU_IRQ_ERROR)

#define STATUS_IDLE 0
#define STATUS_BUSY 1
#define STATUS_DONE 2
#define STATUS_ERROR 3

struct vnpu_device {
	struct pci_dev *pdev;

	void __iomem *base;

	struct completion irq_raised;
	int last_irq;
	struct mutex mutex;

}


static s32 vnpu_get_result(void){
	return (s32)readl(VNPU_REG_RESULT);
}

static u32 vnpu_get_input_a(int base){
	return readl(VNPU_REG_INPUT_A_BASE + (base*4));
}

static u32 vnpu_get_input_b(int base){
	return readl(VNPU_REG_INPUT_B_BASE + (base*4));
}

static void vnpu_set_input_a(u32 value, int base){
	
	writel(value, VNPU_REG_INPUT_A_BASE+(base*4));
}

static void vnpu_set_input_b(u32 value, int base){
	
	writel(value, VNPU_REG_INPUT_B_BASE+(base*4));
}

static int vnpu_hw_reset(struct vnpu_device *vnpu){
	
	u32 status;
	u32 irq_status;
	u32 err;

	wrtiel(0, vnpu->base + VNPU_REG_IRQ_ENABLE);
	writel(VNPU_CONTROL_RESSET, vnpu->base + VNPU_REG_CONTROL);
	writel(VNPU_IRQ_ALL, vnpu->base + VNPU_REG_IRQ_STATUS);

	status = writel(vnpu->base + VNPU_REG_STATUS);
	irq_status = readl(vnpu->base + VNPU_REG_IRQ_STATUS);
	err = readl(vnpu->base + VNPU_REG_ERROR_CODE);	

	if(status != STATUS_IDLE)
			return -EIO;

	if(irq_status & VNPU_IRQ_ALL)
			return -EIO;

	if(err != 0)
			return -EIO;

	reinit_completion(&vnpu->irq_raised);
	vnpu->last_irq = 0;
	return 0;
}

static int vnpu_probe(struct pci_dev *pdev, const struct pci_device_id *id){

	struct vnpu_device vnpu;
	struct device *dev;
	int ret;

	ret = pcim_enable_device(pdev);
	if(ret){
		dev_err(pdev->dev, "Cannot enable PCI device\n");
		return ret;
	}

	vnpu = devm_kzalloc(pdev->dev, sizeof(vnpu), GFP_KERNEL);
	if(!vnpu)
		return -ENOMEM;
	vnpu->pdev = pdev;

	vnpu->base = pcim_iomap(pdev, 0, 0);
	if(!base)
		return -ENOMEM;

	ret = vnpu_hw_reset(vnpu);
	if(ret)
		return ret;

	ret = pci_request_regions(pdev, "vnpu");
	if(ret){
		dev_err(pdev->dev, "Cannot obtaion PCI resources\n");
		return -ENOMEM;
	}

	init_completion(&vnpu->irq_raised);
	mutex_init(&vnpu->mutex);
	
	pci_set_drvdata(pdev, vnpu);
}

static irqreturn_t vnpu_irq_handler(int irq, void *data){

}

static int vnpu_ioctl_proc(struct file *file, unsigned int cmd, unsigned long arg){
	
	u32 res = -ENOTTY;
	switch(cmd){
		case VNPU_IOCTL_GET_INFO:
			struct vnpu_info info;
			if(copy_from_user(&info, (void __user *)arg, sizeof(info)))
				return -EFAULT;
			info->abi_version = readl(VNPU_REG_REVISION);
			info->device_id = readl(VNPU_REG_DEVICE_ID);
			res = 0;
			break;
		case VNPU_IOCTL_RUN_DOT:
			struct vnpu_dot_request dot;
			if(copy_from_user(&dot, (void __user *)arg, sizeof(dot)))
				return -EFAULT;
			for(int i=0; i<4; i++){
				vnpu_set_input_a(dot->input_a[i], i);
				vnpu_set_input_b(dot->input_b[i], i);
			}
			// Need input validation?
			writel(0, VNPU_REG_CONTROL);
			u32 vnpu_status = 0;
			res = readl_poll_timeout(VNPU_REG_STATUS, vnpu_status, vnpu_status & STATUS_DONE, 100, 100000);
			if(res){
				dot->driver_status=readl(VNPU_REG_STATUS);
				dot->device_error=readl(VNPU_REG_ERROR_CODE);
				return -ETIMEDOUT;
			}
			dot->result = vnpu_get_result();
			while(readl(VNPU_REG_STATUS)) {
				if(dot->driver_status == STATUS_ERROR)
					return -EIO;
				if(dot->driver_status == STATUS_DONE){
					dot->result = vnpu_get_result();
					break;
				}
			}
			break;
		case VNPU_IOCTL_RESET:
			break;
		case VNPU_IOCTL_SET_FAULT:
			struct vnpu_fault_request fault;
			if(copy_from_user(&fault, (void __user *)arg, sizeof(fault)))
				return -EFAULT;
			break;
		case VNPU_IOCTL_GET_STAT:
			struct vnpu_status status;
			if(copy_from_user(&status, (void __user *)arg, sizeof(status)))
				return -EFAULT;
			break;
	}

}

static const struct pci_device_id vnpu_pci_id_tbl[] = {
	{PCI_DEVICE(VNPU_VENDOR_ID, VNPU_DEVICE_ID)},
	{}
};

static struct pci_driver vnpu_driver ={
	.name = 	"vnpu",
	.id_table =	vnpu_pci_id_tbl,
	.probe = vnpu_pci_probe,

};
module_pci_driver(vnpu_driver);

struct file_operation vnpu_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = vnpu_ioctl_proc,

};
static int __init vnpu_init(void){

}

static int __exit vnpu_exit(void){

}

module_init(vnpu_init);
module_exit(vnpu_exit);
