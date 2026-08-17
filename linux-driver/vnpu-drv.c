#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#include "vnpu-uapi.h"


#define VNPU_VENDOR_ID 0x1B36
#define VNPU_DEVICE_ID 0x1000

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

	u32 abi_version;

	void __iomem *base;

	unsigned int ioctl_cmd;

	struct completion irq_raised;
	u32 last_irq;
	u32 last_irq_error;
	struct mutex mutex;

	struct miscdevice miscdev;

	u32 device_status;
	u32 device_error;
	u32 result;
	u64 reset_num;
	u64 submitted_num;
	u64 device_error_num;
	u64 completed_num;
	u64 timed_out_num;
};

static irqreturn_t vnpu_irq_handler(int irq, void *data);
static long vnpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static const struct file_operations vnpu_ops;

static void vnpu_set_input_a(void __iomem *base, u32 value, int offset){
	
	writel(value, base + VNPU_REG_INPUT_A_BASE+(offset*4));
}

static void vnpu_set_input_b(void __iomem *base, u32 value, int offset){
	
	writel(value, base + VNPU_REG_INPUT_B_BASE+(offset*4));
}

static int vnpu_hw_reset(struct vnpu_device *vnpu){
	
	u32 status;
	u32 irq_status;
	u32 err;

	writel(0, vnpu->base + VNPU_REG_IRQ_ENABLE);
	writel(VNPU_CONTROL_RESET, vnpu->base + VNPU_REG_CONTROL);
	writel(VNPU_IRQ_ALL, vnpu->base + VNPU_REG_IRQ_STATUS);

	status = readl(vnpu->base + VNPU_REG_STATUS);
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

	struct vnpu_device *vnpu;
	int ret;
	
	/*
	 *	device 활성화 체크
	 */
	ret = pcim_enable_device(pdev);
	if(ret){
		dev_err(&pdev->dev, "Cannot enable PCI device\n");
		return ret;
	}

	/*
	 *	device 용 메모리 할당
	 */
	vnpu = devm_kzalloc(&pdev->dev, sizeof(*vnpu), GFP_KERNEL);
	if(!vnpu)
		return -ENOMEM;
	vnpu->pdev = pdev;
	vnpu->dev = &pdev->dev;
	
	ret = pci_request_regions(pdev, "vnpu");
	if(ret){
		dev_err(&pdev->dev, "Cannot obtaion PCI resources\n");
		return -ENOMEM;
	}
	/*
	 * device 용 BAR 확인 이후 값 저
	 */
	vnpu->base = pcim_iomap(pdev, 0, 0);
	if(!vnpu->base)
		return -ENOMEM;

	init_completion(&vnpu->irq_raised);
	mutex_init(&vnpu->mutex);

	/*
	 * device의 모든 resource 할당.
	 */
	ret = vnpu_hw_reset(vnpu);
	if(ret)
		return ret;


	
	ret = devm_request_irq(&pdev->dev, pdev->irq, vnpu_irq_handler, IRQF_SHARED, dev_name(&pdev->dev), vnpu);
	if(ret) {
		dev_err(&pdev->dev, "failed to request irq\n");
		return ret;
	}

	/*
	 * file node 등록 및 노출
	 */
	vnpu->miscdev.minor = MISC_DYNAMIC_MINOR;
	vnpu->miscdev.name = "vnpu0";
	vnpu->miscdev.fops = &vnpu_ops;
	vnpu->miscdev.parent = &pdev->dev;
	vnpu->miscdev.mode = 0666;

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
	
	printk("probe success\n");
	return 0;
}

static void vnpu_remove(struct pci_dev *pdev){

	struct vnpu_device *vnpu = pci_get_drvdata(pdev);
	misc_deregister(&vnpu->miscdev);
	pci_release_regions(pdev);

}

static irqreturn_t vnpu_irq_handler(int irq, void *data){
	
	struct vnpu_device *vnpu = data;
	int irq_status = readl(vnpu->base + VNPU_REG_IRQ_STATUS);
	writel(VNPU_IRQ_ALL, vnpu->base + VNPU_REG_IRQ_STATUS);

	if(!(irq_status & VNPU_IRQ_ALL)){
		return IRQ_NONE;
	}
		
	if(irq_status & VNPU_IRQ_ERROR){
		vnpu->device_error = readl(vnpu->base + VNPU_REG_ERROR_CODE);
		vnpu->device_status = readl(vnpu->base + VNPU_REG_STATUS);
		vnpu->device_error_num += 1;
	}

	complete(&vnpu->irq_raised);

	return IRQ_HANDLED;
}

static long vnpu_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	
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

			info.abi_version = 1;
			info.device_id = readl(VNPU_REG_DEVICE_ID);
			res = 0;
			
			if(copy_to_user((void __user *)arg, &info, sizeof(info)))
				return -EFAULT;

			break;
		case VNPU_IOCTL_RUN_DOT:
			struct vnpu_dot_request dot;
			int vec_len = 0;
			
			if(copy_from_user(&dot, (void __user *)arg, sizeof(dot)))
				return -EFAULT;

			if(dot.abi_version != 1){
				dev_err(vnpu->dev, "invalid revistion version\n");
				return -EINVAL;
			}

			vec_len = readl(vnpu->base + VNPU_REG_VECTOR_LENGTH);
			if(dot.vector_length != vec_len){
				dev_err(vnpu->dev, "invalid length\n");
				return -EINVAL;
			}

			if(dot.timeout_ms == 0 || dot.timeout_ms > 1000){
				dev_err(vnpu->dev, "invalid timeout ms");
				return -EINVAL;
			}
			timeout = msecs_to_jiffies(dot.timeout_ms);

			mutex_lock(&vnpu->mutex);
			reinit_completion(&vnpu->irq_raised);

			//CLEAR STATUS
			writel(VNPU_IRQ_ALL, vnpu->base + VNPU_REG_IRQ_STATUS);
			writel(VNPU_IRQ_ALL, vnpu->base + VNPU_REG_IRQ_ENABLE);

			for(int i=0; i<4; i++){
				vnpu_set_input_a(vnpu->base, dot.input_a[i], i);
				vnpu_set_input_b(vnpu->base, dot.input_b[i], i);
			}
			writel(dot.vector_length, vnpu->base + VNPU_REG_VECTOR_LENGTH);
			writel(VNPU_CONTROL_START, vnpu->base + VNPU_REG_CONTROL);
			vnpu->submitted_num += 1;
			
			remaining = wait_for_completion_timeout(&vnpu->irq_raised, timeout);
			
			if(!remaining){
				
				writel(VNPU_CONTROL_RESET, vnpu->base + VNPU_REG_CONTROL);
				dot.driver_status = DRIVER_STATUS_TIMEOUT;
				vnpu->timed_out_num += 1;
				
				mutex_unlock(&vnpu->mutex);
				
				if(copy_to_user((void __user *)arg, &dot, sizeof(dot)))
					return -EFAULT;

				return -ETIMEDOUT;
			}

			dot.result = readl(vnpu->base + VNPU_REG_RESULT);
			dot.driver_status = DRIVER_STATUS_OK;
			vnpu->completed_num += 1;
			mutex_unlock(&vnpu->mutex);
			if(copy_to_user((void __user *)arg, &dot, sizeof(dot)))
				return -EFAULT;
			break;
		case VNPU_IOCTL_RESET:
			mutex_lock(&vnpu->mutex);
			
			res = vnpu_hw_reset(vnpu);
			if(!res)
				vnpu->reset_num += 1;
			
			mutex_unlock(&vnpu->mutex);
			break;
		case VNPU_IOCTL_SET_FAULT:
			struct vnpu_fault_request fault;

			if(copy_from_user(&fault, (void __user *)arg, sizeof(fault)))
				return -EFAULT;
			
			mutex_lock(&vnpu->mutex);
			
			fault.fault_mask &= 0xf;
			writel(fault.fault_mask , vnpu->base + VNPU_REG_FUALT_CONTROL);
			res = 0;
			
			mutex_unlock(&vnpu->mutex);

			if(copy_to_user((void __user *)arg, &fault, sizeof(fault)))
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
			if(copy_to_user((void __user *)arg, &status, sizeof(status)))
				return -EFAULT;
			break;
	}
	return res;

}

static const struct pci_device_id vnpu_pci_id_tbl[] = {
	{PCI_DEVICE(VNPU_VENDOR_ID, VNPU_DEVICE_ID)},
	{}
};

static struct pci_driver vnpu_driver ={
	.name = 	"vnpu",
	.id_table =	vnpu_pci_id_tbl,
	.probe = vnpu_probe,
	.remove = vnpu_remove,

};

static const struct file_operations vnpu_ops = {
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
