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
	struct device *dev;

	void __iomem *base;

	unsigned int ioctl_cmd;

	struct completion irq_raised;
	u32 last_irq;
	u32 last_irq_error;
	struct mutex mutex;

	struct miscdevice *miscdev;

	u32 device_status;
	u32 device_error;
	u32 result;
	u64 reset_num;
	u64 submitted_num;
	u64 device_error_num;
	u64 completed_num;
	u64 timed_out_num;
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
	writel(VNPU_CONTROL_RESET, vnpu->base + VNPU_REG_CONTROL);
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
	
	/*
	 *	device 활성화 체
	 * 크
	 */
	ret = pcim_enable_device(pdev);
	if(ret){
		dev_err(pdev->dev, "Cannot enable PCI device\n");
		return ret;
	}

	/*
	 *	device 용 메모리 할당
	 */
	vnpu = devm_kzalloc(pdev->dev, sizeof(vnpu), GFP_KERNEL);
	if(!vnpu)
		return -ENOMEM;
	vnpu->pdev = pdev;
	
	ret = pci_request_regions(pdev, "vnpu");
	if(ret){
		dev_err(pdev->dev, "Cannot obtaion PCI resources\n");
		return -ENOMEM;
	}
	/*
	 * device 용 BAR 확인 이후 값 저
	 */장
	vnpu->base = pcim_iomap(pdev, 0, 0);
	if(!base)
		return -ENOMEM;

	/*
	 * device의 모든 resource 할당.
	 */
	ret = vnpu_hw_reset(vnpu);
	if(ret)
		return ret;


	init_completion(&vnpu->irq_raised);
	mutex_init(&vnpu->mutex);
	
	devm_request_irq(&pdev->dev, pdev->irq, vnpu_irq_hadnler, IROF_SHARED, dev_name(&pdev->dev), vnpu)

	/*
	 * file node 등록 및 노출
	 */
	vnpu->miscdev.minor = MISC_DYNAMIC_MINOR;
	vnpu->miscdev.name = "vnpu0";
	vnpu->midsdev.fops = &vnpu_ops;
	vnpu->miscdev.parent = &pdev->dev;

	ret = misc_register(&vnpu->miscdev);
	if(ret){
		dev_err(&pdev->dev, "failed to register misc device: %d\n", ret);
		return ret;
	}

	vnpu->device_error_num = 0;
	vnpu->reset_num = 0;
	vnpu->timed_out_num = 0;
	vnpu->submitted_num = 0;
	vnpu->completed_num = 0;

	pci_set_drvdata(pdev, vnpu);

	return 0;
}

static void vnpu_remove(sstruct pci_dev *pdev){

	struct vnpu_device *vnpu = pci_get_drvdata(pdev);
	misc_deregister(&vnpu->miscdev);

}

static irqreturn_t vnpu_irq_handler(int irq, void *data){
	
	int res = 0;
	struct vnpu_device *vnpu = data;

	if(irq == VNPU_IRQ_ERROR){
		vnpu->device_error = readl(vnpu->base + VNPU_REG_ERROR_CODE);
		vnpu->device_status = readl(vnpu->base + VNPU_REG_STATUS);
		vnpu->device_error_num += 1;
	}

	complete(vnpu->irq_raised);

	return IRQ_HANDLED;
}

static int vnpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	
	struct miscdevice *miscdev = file->private_data;
	struct vnpu_device *vnpu;
	u32 res = -ENOTTY;

	unsigned long timeout;
	long remaining;
	
	vnpu = container_of(miscdev, struct vnpu_device, miscdev);

	switch(cmd){
		case VNPU_IOCTL_GET_INFO:
			struct vnpu_info info;
			if(copy_from_user(&info, (void __user *)arg, sizeof(info)))
				return -EFAULT;
			info.abi_version = readl(VNPU_REG_REVISION);
			info.device_id = readl(VNPU_REG_DEVICE_ID);
			res = 0;
			
			if(copy_to_user((void __user *)arg), &info, sizeof(info));
				return -EFAULT;
			break;
		case VNPU_IOCTL_RUN_DOT:
			struct vnpu_dot_request dot;
			if(copy_from_user(&dot, (void __user *)arg, sizeof(dot)))
				return -EFAULT;
			
			timeout = msecs_to_jiffies(dot.timeout_ms);

			for(int i=0; i<4; i++){
				vnpu_set_input_a(dot.input_a[i], i);
				vnpu_set_input_b(dot.input_b[i], i);
			}
			writel(vnpu->vector_length, vnpu->base + VNPU_REG_VECTOR_LENGTH);
			writel(VNPU_CONTROL_START, vnpu->base + VNPU_REG_CONTROL);
			vnpu->submitted_num += 1;
			wait_for_completion_timeout(vnpu->irq_raised, timeout);
			if(!remaining){
				writel(VNPU_CONTROL_RESET, vnpu->base + VNPU_REG_CONTROL);
				dot.driver_status = DRIVER_STATUS_TIMEOUT;
				vnpu->timed_out_num += 1;
				return -ETIMEDOUT;
			}

			dot.result = readl(vnpu->base + VNPU_REG_RESULT);
			dot.driver_status = DRIVER_STATUS_OK;
	
			if(copy_to_user((void __user *)arg), &dot, sizeof(dot));
				return -EFAULT;
			vnpu->completed_num += 1;
			break;
		case VNPU_IOCTL_RESET:
			vnpu->ioctl_cmd = VNPU_IOCTL_RESET;
			writel(VNPU_CONTROL_RESET, vnpu->base + VNPU_REG_CONTROL);
			wait_for_completion(vnpu->irq_raised);
			vnpu->reset_num += 1;
			break;
		case VNPU_IOCTL_SET_FAULT:
			struct vnpu_fault_request fault;
			if(copy_from_user(&fault, (void __user *)arg, sizeof(fault)))
				return -EFAULT;
			writel(fault.fault_mask, vnpu->base + VNPU_REG_FUALT_CONTROL);
			wait_for_completion(vnpu->irq_raised);
			res = 0;
			if(copy_to_user((void __user *)arg), &fault, sizeof(fault));
				return -EFAULT;
			break;
		case VNPU_IOCTL_GET_STAT:
			struct vnpu_status status;
			if(copy_from_user(&status, (void __user *)arg, sizeof(status)))
				return -EFAULT;
			status.timed_out = vnpu->timed_out_num;
			status.completed = vnpu->completed_num;
			status.device_error = vnpu->device_error_num;
			status.submitted = vnpu->submitted_num;
			status.resets = vnpu->reset_num;
			res = 0;
			if(copy_to_user((void __user *)arg), &status, sizeof(status));
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
	.remove = vnpu_remove,

};

struct file_operation vnpu_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = vnpu_ioctl,
	/* .compat_ioctl = vnpu_ioctl, */

};

module_pci_driver(vnpu_driver);
MODULE_DEVICE_TABLE(pci, vnpu_pci_id_tbl);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yoo");
MODULE_DESCRIPTION("VNPU PCI driver");
/* static int __init vnpu_init(void){ */
/*  */
/* } */
/*  */
/* static int __exit vnpu_exit(void){ */
/*  */
/* } */
/*  */
/* module_init(vnpu_init); */
/* module_exit(vnpu_exit); */
