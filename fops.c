#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include "scull.h"

#define CHECK_PERM			\
	if(!capable(CAP_SYS_ADMIN))	\
		return -EPERM;

int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next,*dptr;
	int qset = dev->qset;
	
	int i;	
	for(dptr = dev->data; dptr; dptr = next) {
		if(dptr->data) {
			for(i=0;i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;
	return 0;
}

int scull_open(struct inode *inode,struct file *filp)
{
	struct scull_dev *dev;
	dev = container_of(inode->i_cdev,struct scull_dev,cdev);
	filp->private_data = dev;
	
	if((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		scull_trim(dev);
	}
	return 0;
}

int scull_release(struct inode *inode,struct file *filep)
{
	return 0;
}
ssize_t scull_read(struct file *filp,char __user *buf,size_t count,loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	/* Critical Section: using seamphore to 
	   avoid race conditions b/w threads 
	   accessing same data structures. Since our
	   scull device is not holding any other resource
	   which it need to release so it can go to sleep.
	   Therefore that locking mechanism should be used 
	    in which process can sleep hence SEMAPHORES.
	
          P functions are called "down" which has 3 variations:
	   => down() : decrements the value of semaphore and wait
		       as long as needed.
           => down_interruptible() : decrements the value of semaphore
				     but the operation is interruptible
	   => down_trylock : if semaphore not availble at time of call
			    , returns immediately.
	   P functions calling indicate that thread is ready to execute
	   in critical section */	

	if(down_interruptible(&dev->sem)) 
		return -ERESTARTSYS;
	if(*f_pos >= dev->size)
		goto out;
	if(*f_pos + count > dev->size ) 
		count = dev->size - *f_pos;
	
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;
	
	dptr = scull_follow(dev,item);
	
	if(dptr == NULL || !dptr->data || !dptr->data[s_pos] ) 
		goto out;
	
	if(count > quantum - q_pos)
		count = quantum - q_pos;
	
	if(copy_to_user(buf,dptr->data[s_pos] + q_pos,count)) {
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;
	
	out:
		/* release the semaphore since the mutex section is
		   complete */
		up(&dev->sem);
		return retval;
	
}

ssize_t scull_write(struct file *filp,const char __user *buf,size_t count,
			loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if(down_interruptible(&dev->sem)) 
		return -ERESTARTSYS;
	
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;

	s_pos = rest / quantum;
	q_pos = rest % quantum;
		
	dptr = scull_follow(dev,item);
	
	if(dptr == NULL)
		goto out;
	
	if(!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *),GFP_KERNEL);
		
		if(!dptr->data)
			goto out;
		memset(dptr->data,0,qset * sizeof(char *));		
	}
	
	if(!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum,GFP_KERNEL);

		if(!dptr->data[s_pos])
			goto out;
	}
	
	if(count > quantum - q_pos )
		count = quantum - q_pos;
	
	if(copy_from_user(dptr->data[s_pos] + q_pos,buf,count)) {
		retval = -EFAULT;
		goto out;
	}
	
	*f_pos += count;
	retval = count;
	
	if(dev->size  < *f_pos )
		dev->size = *f_pos;

	out:
		up(&dev->sem);
		return retval;
}

int scull_ioctl(struct file *filp,unsigned int cmd,unsigned long arg) 
{
	
	int retval = 0;
	int tmp    = 0;
	int err	   = 0;

	/* extract the type and number bitfields, and dont
	   decode wrong cmds: return ENOTTY before access_ok()
	    which in user space is interpreted as INVALID CMD
	*/
	if(_IOC_TYPE(cmd) != SCULL_IOC_MAGIC)
		return -ENOTTY;
	if(_IOC_NR(cmd) > SCULL_IOC_MAXNR)
		return -ENOTTY;

	/*The direction field is a bitmask (2 bits), and 
	VERIFY_WRITE catches R/W transfers. 'direction'
	bitfield is user-oriented, while access_ok() is
	kernel-oriented so direction bitfields are reversed
	hence the conecpt of read and write.
	
 	32 bit bitfield: 31st and 30th bits are direction bits

	00 : IO
	10 : IOR
	01 : IOW
	11 : IORW
	
	so for eg if in user space direction bits are 10 in kernel
	it will be 01. hence read and write are reversed
	
	*/
	
	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user*)arg,_IOC_SIZE(cmd));
	
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,(void __user*)arg,_IOC_SIZE(cmd));

	if(err)
		return -EFAULT;

	switch(cmd) {

		case SCULL_IOCRESET:
			scull_quantum = SCULL_QUANTUM;	
			scull_qset    = SCULL_QSET;
			break;

		case SCULL_IOCSQUANTUM:    // Set: arg points to value
			CHECK_PERM;
			retval = __get_user(scull_quantum,(int __user*)arg);
			break;

		case SCULL_IOCSQSET:
			CHECK_PERM;
			retval = __get_user(scull_qset,(int __user*)arg);
			break;

		case SCULL_IOCTQUANTUM:   // Tell: arg is the value
			CHECK_PERM;
			scull_quantum = arg;
			break;

		case SCULL_IOCTQSET:	
			CHECK_PERM;
			scull_qset = arg;
			break;

		case SCULL_IOCGQUANTUM:	   // Get: arg is pointer to result
			retval = __put_user(scull_quantum,(int __user*)arg);
			break;

		case SCULL_IOCGQSET:
			retval = __put_user(scull_quantum,(int __user*)arg);
			break;
		
		case SCULL_IOCQQUANTUM:   // Query: return value is result
			return scull_quantum;

		case SCULL_IOCQQSET:
			return scull_qset;
 
		case SCULL_IOCXQUANTUM:   // Xchange: use arg as pointer
			CHECK_PERM;
			tmp = scull_quantum;
			retval = __get_user(scull_quantum,(int __user*)arg);
			if(retval == 0)
				retval = __put_user(tmp,(int __user *)arg);
			break;

		case SCULL_IOCXQSET:
			CHECK_PERM;
			tmp = scull_qset;
			retval = __get_user(scull_qset,(int __user*)arg);
			if(retval == 0)
				retval = __put_user(tmp,(int __user*)arg);
			break;

		case SCULL_IOCHQUANTUM:	// Shift: Tell + Query
			CHECK_PERM;
			tmp = scull_quantum;
			scull_quantum = arg;
			return tmp;
	
		case SCULL_IOCHQSET:
			CHECK_PERM;
			tmp = scull_qset;
			scull_qset = arg;
			return tmp;

		default:
			return -ENOTTY;
	}
	return retval;

}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.read  = scull_read,
	.write = scull_write,
	.open  = scull_open,
	.release = scull_release
};
