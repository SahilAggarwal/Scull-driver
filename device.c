#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "scull.h"

dev_t create_dev(void)
{
	dev_t dev;
	int result = -1;
	if(scull_major) {
		dev = MKDEV(scull_major,scull_minor);
		result = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs,"scull" );
		scull_major = MAJOR(dev);
	}

	if(result < 0 ) {
		printk(KERN_WARNING "scull: could not get major number");
		return result;
	}
	return dev;
}

struct scull_qset *scull_follow(struct scull_dev *dev,int n)
{
	struct scull_qset *qs = dev->data;
		
	if(!qs) {
		qs = dev->data = kmalloc(sizeof(struct scull_qset),GFP_KERNEL);
		
		if(qs == NULL)	
			return NULL;
		memset(qs,0,sizeof(struct scull_qset));
	}
	
	while(n--) {
		if( !qs->next ) {
			qs->next = kmalloc(sizeof(struct scull_qset),GFP_KERNEL);

			if(qs->next == NULL)
				return NULL;	
			memset(qs,0,sizeof(struct scull_qset));	
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

static void *scull_seq_start(struct seq_file *s,loff_t *pos)
{
	if(*pos >= scull_nr_devs )
		return NULL;
	return scull_devices + *pos;
}

static void *scull_seq_next(struct seq_file *s,void *v,loff_t *pos)
{
	(*pos)++;
	if(*pos >= scull_nr_devs)
		return NULL;
	return scull_devices + *pos;
}

static void scull_seq_stop(struct seq_file *s,void *v)
{

}

static int scull_seq_show(struct seq_file *s, void *v)
{
	struct scull_dev *dev = (struct scull_dev *)v;
	struct scull_qset *d;
	int i;
	
	if(down_interruptible(&dev->sem)) 
		return -ERESTARTSYS;
	
	seq_printf(s,"\nDevice %i: qser %i, q %i, sz %li\n",
		   (int)(dev - scull_devices),dev->qset,
		   dev->quantum,dev->size);

	for(d= dev->data; d; d=d->next) {
		seq_printf(s,"	 item at %p,qset at %p\n",d,d->data);
		if(d->data && !d->next) {
			for(i=0; i < dev->qset; i++ ) {
				if(d->data[i])
					seq_printf(s,"         %4i: %8p\n",
								i,d->data[i]);	
			}
		}
	}
	up(&dev->sem);
	return 0;
}

static struct seq_operations scull_seq_ops = {
	.start = scull_seq_start,
	.stop  = scull_seq_stop,
	.show  = scull_seq_show,
	.next  = scull_seq_next
};

static int scull_proc_open(struct inode *inode,struct file *file)
{
	return seq_open(file,&scull_seq_ops);
}

static struct file_operations scull_proc_ops = {
	.owner = THIS_MODULE,
	.open = scull_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

void procfile_setup(void)
{	
	proc_create_data("scullseq",0,NULL,&scull_proc_ops,NULL);
}
