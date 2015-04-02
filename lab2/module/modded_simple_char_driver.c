

#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#include<linux/syscalls.h>
#define BUFFER_SIZE 1024

static char device_buffer[BUFFER_SIZE];
static char disk_buffer[256];
int opencount = 0;
int closecount = 0;
int eof = 0;
loff_t pos = 0;
int i;
int bread, bwrite;
mm_segment_t oldfs;
struct file* infile; 
struct file* outfile;

ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_to_user function. source is device_buffer (the buffer defined at the start of the code) and destination is the userspace 		buffer *buffer */
	if(eof == -1) return 0;
	bread = length < eof - (*offset) ? length : eof - (*offset);
	printk(KERN_ALERT "simple_char_driver: %d bytes read\n", bread);
	copy_to_user(buffer, device_buffer, bread);
	(*offset) += bread;
	return bread;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_from_user function. destination is device_buffer (the buffer defined at the start of the code) and source is the userspace 		buffer *buffer */

	if (eof == BUFFER_SIZE) return -1;
	bwrite = BUFFER_SIZE - eof > (length) ? (length) : BUFFER_SIZE - eof;
	printk(KERN_ALERT "simple_char_driver: %d bytes written\n", bwrite);
	copy_from_user((device_buffer + eof), buffer, bwrite);
	eof += length;
	
	//oldfs = get_fs();
	//set_fs(KERNEL_DS);
	pos = 0;
	
	infile = filp_open("/tmp/virtual_dev_out", O_RDONLY, 0644);
	outfile = filp_open("/tmp/virtual_dev_in", O_WRONLY, 0644);
	
	if(!infile || !outfile) return length;
	
	vfs_write(outfile, "WRITE_REQ", 10, &pos);
	pos = 0;
	vfs_read(infile, disk_buffer, 256, &pos);
	/*while(strcmp(disk_buffer, "WRITE_ACK") != 0) {
		pos = 0;
		vfs_read(infile, disk_buffer, 256, &pos);
	}
	pos = eof - length;
	vfs_write(outfile, device_buffer, length, &pos);*/
	printk(KERN_ALERT "simple char driver: received %s\n", disk_buffer);
	
	filp_close(infile, NULL);
	filp_close(outfile, NULL);
	//set_fs(oldfs);
	
	return length;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	printk(KERN_ALERT "simple device opened! opened: %d times\n", ++opencount);
	return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	printk(KERN_ALERT "simple device closed! closed: %d times\n", ++closecount);
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	.open	 = simple_char_driver_open,
	.release = simple_char_driver_close,
	.write	 = simple_char_driver_write,
	.read	 = simple_char_driver_read
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	/* register the device */
	printk(KERN_ALERT "%s function called\n", __FUNCTION__);
	register_chrdev(150, "simple_char_driver", &simple_char_driver_file_operations);
	return 0;
}

static int simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	/* unregister  the device using the register_chrdev() function. */
	printk(KERN_ALERT "%s function called\n", __FUNCTION__);
	unregister_chrdev(150, "simple_char_driver");
	return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);

