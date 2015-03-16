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
