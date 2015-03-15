#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include "scull.h"

MODULE_LICENSE("GPL");

unsigned int scull_major = SCULL_MAJOR;
unsigned int scull_minor = SCULL_MINOR;
unsigned int scull_nr_devs = SCULL_NR_DEVS;
unsigned int scull_quantum = SCULL_QUANTUM;
unsigned int scull_qset = SCULL_QSET;

struct scull_dev *scull_devices;
static dev_t dev;

static void scull_setup_dev(struct scull_dev *dev,int index)
{
	int err, devno = MKDEV(scull_major,scull_minor + index);
	cdev_init(&dev->cdev,&scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops   = &scull_fops;
	err = cdev_add(&dev->cdev,devno,1);
	
	if(err)
		printk(KERN_NOTICE "Error %d adding scull %d",err,index);
	
}

static int __init scull_init(void)
{
	int i;
	printk(KERN_INFO "scull: initializing ...");
	if((dev = create_dev()) == -1){
		printk(KERN_INFO "scull: initialization failed\n");
		goto out;
	}
	
	scull_devices = kmalloc(sizeof(struct scull_dev)*scull_nr_devs,GFP_KERNEL);
	memset(scull_devices,0,sizeof(struct scull_dev)*scull_nr_devs);

	for(i=0;i<scull_nr_devs;i++) {
		sema_init(&scull_devices[i].sem,1);
		scull_setup_dev(&scull_devices[i],i);
	}
	procfile_setup();
	
	printk(KERN_INFO "scull: initialized\n");
	
	return 0;
	out:
		return -1;
}

void __exit scull_exit(void)
{
	printk(KERN_INFO "scull: unitializiing ...\n");
	unregister_chrdev_region(dev,scull_nr_devs);
	printk(KERN_INFO "scull: unitialized\n");
	
}

module_init(scull_init);
module_exit(scull_exit);


