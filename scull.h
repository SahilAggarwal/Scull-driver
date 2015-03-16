#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/ioctl.h>


#define SCULL_MAJOR   0
#define SCULL_MINOR   0
#define SCULL_NR_DEVS 4
#define SCULL_QUANTUM 4000
#define SCULL_QSET    1000

extern unsigned int scull_major;
extern unsigned int scull_minor;
extern unsigned int scull_nr_devs;
extern unsigned int scull_quantum;
extern unsigned int scull_qset;

extern struct scull_dev *scull_devices;
/*
   scull_dev    scull_qset
   ______       ______  next
  |      |     |      |=======>
  | data |====>| data |====||
  |______|     |______|    ||
  			   ||
			-----------  Size of each Quantum =4000
			-----------
			-----------
			-----------
			-----------
			-----------
			    ;
			    ;
			  1000 Quantums
*/



struct scull_dev {
	struct scull_qset *data;   // Pointer to data
	int quantum;		   // Quantum size
	int qset;                  // No of quantums in each struct
	unsigned long size;        // Total size of device
	unsigned int access_key;   // used by sculluid and scullpriv(Beginners 
				   // can igore this)
	struct semaphore sem;      // mutex semaphore
	struct cdev cdev;          // char device structure
};

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

extern void procfile_setup(void);

extern dev_t create_dev(void);

struct scull_qset *scull_follow(struct scull_dev *, int);

extern struct file_operations scull_fops;

/* ioctl are used to control the hardware parameters
   each ioctl command is associated with cmd number which
   has to be unique across the system.
   So to create a unique cmd number you need

   1) Magic number which is same across your driver
   2) Sequence number ( 0..n)
   3) Type of data going into or coming out of kernel(optional)

   _IO  an ioctl with no parameters
   _IOW an ioctl with write parameters (copy_from_user) 
   _IOR an ioctl with read parameters  (copy_to_user)
   _IOWR an ioctl with both write and read params

   see Documentation/ioctl/ioctl-numbers.txt for more details on this.
    

*/


#define SCULL_IOC_MAGIC 		0xF7
#define SCULL_IOCRESET 		_IO(SCULL_IOC_MAGIC,0)

/*
	o IOCS means 'Set'  	through a ptr
	o ___T means 'Tell' 	directly with the arg value
	o ___G means 'Get'  	reply by setting through ptr
	o ___Q means 'Query'	response is on the return value
	o ___X means 'Xchange'	G+S i.e set new value through ptr and
				return old value through same ptr
	o ___H means 'Shift'	T+Q i.e set value as arg and return 
				old value
*/


#define SCULL_IOCSQUANTUM	_IOW(SCULL_IOC_MAGIC, 1,int)
#define SCULL_IOCSQSET		_IOW(SCULL_IOC_MAGIC, 2,int)
#define SCULL_IOCTQUANTUM	_IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET		_IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCGQUANTUM	_IOR(SCULL_IOC_MAGIC, 5,int)
#define SCULL_IOCGQSET		_IOR(SCULL_IOC_MAGIC, 6,int)
#define SCULL_IOCQQUANTUM	_IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET		_IO(SCULL_IOC_MAGIC, 8)
#define SCULL_IOCXQUANTUM	_IOWR(SCULL_IOC_MAGIC, 9,int)
#define SCULL_IOCXQSET		_IOWR(SCULL_IOC_MAGIC, 10,int)
#define SCULL_IOCHQUANTUM	_IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET		_IO(SCULL_IOC_MAGIC, 12)

#define SCULL_IOC_MAXNR 14
