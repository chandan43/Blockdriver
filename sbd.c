#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/types.h>  /* size_t */
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */

#include <linux/genhd.h>
#include <linux/vmalloc.h>  

#include <linux/blkdev.h>

#define KERNEL_SECTOR_SIZE 512
#define DEVICE_NAME "sbd"
#define MINOR 16
static int sbd_getgeo(struct block_device *, struct hd_geometry *);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("beingchandanjha@gamil.com");
MODULE_DESCRIPTION("My first basic Block Device Driver");
MODULE_VERSION(".1");

/*KERNEL_SECTOR_SIZE is locally defined constant that we use to scale between the kernel's 512 bytes sectors and whatever size we have been told to use.*/


static unsigned int majornumber=0;
module_param(majornumber,uint,0);
MODULE_PARM_DESC(majornumber,"Major number of Block driver:it should be unsigned integer in between 0 to 255.");
static unsigned int logical_block_size=512;        /*Setctor Size*/ 
module_param(logical_block_size,uint,0);
MODULE_PARM_DESC(logical_block_size,"Logical Block size : it should be power of 2 (2^n) and less then page size.By default 512 bytes.");
static unsigned int nsector=1024;         /*Size of Driver i.e total no sector */
module_param(nsector,uint,0);
MODULE_PARM_DESC(nsector,"Sector Size : Default 1024");

/* Internal structure of block device */
typedef struct sbd_dev{
	int size;                 /*Device size in sectors*/
	u8 *data;                 /* Data Array */ 
	struct gendisk *gd;       /* The gendisk structure */
	struct request_queue *Queue;  /*Device request queue */
	spinlock_t lock;
}Device;
Device *dev;

void sbd_req(){
	pr_info("exit\n");
}
static struct block_device_operations sdb_fops={
	.owner   = THIS_MODULE,
	.getgeo  =sbd_getgeo;
};

int sbd_init(void){
	pr_info("%s: Initialization of Block Device Driver\n",__func__);
	dev->size=nsector*logical_block_size;
	spin_lock_init(&dev->lock);
	dev->data=vmalloc(dev->size);
	if(dev->data==NULL){
		pr_err("%s: Insufficient memory\n",__func__);
		return -ENOMEM;
	}
	/*allocation of Request queue for request processing */
	dev->Queue=blk_init_queue(sbd_req,&dev->lock);
	if(dev->Queue==NULL){
		pr_err("blk_init_queue: queue Initialization Failed\n");
		goto free;
	}
	/*set logical_block_size for Initialised Queue*/
	blk_queue_logical_block_size(dev->Queue,logical_block_size);
	/*Block Device Registration */
	majornumber=register_blkdev(0,DEVICE_NAME);
	if(majornumber<0){
		pr_err("%s: Device Registration failed\n",__func__);
		goto free;
	}
	/*Initialised of gendisk structure */
	dev->gd=alloc_disk(MINOR);
	if(!dev->gd){
		pr_err("Gendisk: allocation failed-%s.\n",__LINE__);
		goto free;
	}
	dev->gd->major=majornumber;
	dev->gd->first_minor=0;
	dev->gd->fops=&sdb_fops;
	dev->gd->private_data=&dev;
	strcpy(dev->gd->disk_name,"sbd0");
	set_capacity(gd,nsector);
	dev->gd->queue=dev->Queue;
	add_disk(gd);
	return 0;
unreg_dev:
	unregister_blkdev(majornumber,DEVICE_NAME);
free: 
	vfree(dev->data);
	return -ENOMEM;
}

void sbd_exit(void){
	pr_info("%s: Block Device Driver Exited successfully\n",__func__);
}

module_init(sbd_init);
module_exit(sbd_exit);

