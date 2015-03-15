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

struct scull_dev {
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int access_key;
	struct semaphore sem;
	struct cdev cdev;
};

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

extern void procfile_setup(void);

extern dev_t create_dev(void);

struct scull_qset *scull_follow(struct scull_dev *, int);

extern struct file_operations scull_fops;
