#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define I2C_SLAVE_ADDR 0x68
#define REG_VERSION    0x11

#define I2C_DRIVER_NAME "i2c_sensor"
#define MODNAME "sensor"

struct sensor_device_desr {
    struct mutex        mutex;
    struct cdev         cdev;
    int                 major;
    struct i2c_client   *client;
    char                version;
    int                 read;
};

static struct sensor_device_desr sensor_device;

//board info
static struct i2c_board_info sensor_i2c_board_info = {
	I2C_BOARD_INFO("sensor", I2C_SLAVE_ADDR)
};

static struct i2c_client *sensor_client;

/*-------------------- file operations --------------------------------------*/

static int sensor_open(struct inode *inode, struct file *file) {
	struct sensor_device_desr *dev;

	dev = container_of(inode->i_cdev, struct sensor_device_desr, cdev);
	file->private_data = dev;
	dev->read = 1;     //////???????????

	return 0;
}

static int sensor_release(struct inode *inode, struct file *file) {
	return 0;
}

static int read_register(struct i2c_client *client, char reg) {
	//int ret;
	//int8_t t;

	return i2c_smbus_read_byte_data(client, reg);
}

static ssize_t sensor_read(struct file *file, char __user *buffer, size_t count, loff_t *offset) {
	struct sensor_device_desr *dev = file->private_data;
	ssize_t ret = 0;
	int data_read;

	if (mutex_lock_interruptible(&dev->mutex)) return -EINTR;

	//read version
	data_read = read_register(dev->client, REG_VERSION);

	dev->version = data_read;
	dev->read = 1;

	//copy to user ???????????
	
	return ret;

}

static struct file_operations sensor_fops = {
	.owner    = THIS_MODULE,
	.open     = sensor_open,
	.release  = sensor_release,
	.read     = sensor_read
};

/*-------------------- table id ---------------------------------------------*/
static const struct i2c_device_id sensor_id[] = {
	{"sensor",              /*name*/
	 I2C_SLAVE_ADDR         /*driver_data*/
	},
	{}                      /*empty*/
};
	
MODULE_DEVICE_TABLE(i2c, sensor_id);

/*-----------------------device probe----------------------------------------*/

static int sensor_probe(struct i2c_client *client, const struct i2c_device_id *id) {
	dev_t devid = 0;
	int err, devno;
	struct sensor_device_desr *dev;

	if (client->addr != id->driver_data) {
		pr_err(MODNAME ": wrong address (is %d)\n", client->addr);
		return -ENODEV;
	}

	memset(&sensor_device, 0, sizeof(sensor_device));
	dev = &sensor_device;     /*or use dynamic allocation*/
	i2c_set_clientdata(client, dev);

	/*Allocation of major numer*/
	dev->client = client;
	err = alloc_chrdev_region(&devid, 0, 1, MODNAME);
	dev->major = MAJOR(devid);
	if (err < 0) {
		pr_warning(KERN_WARNING MODNAME ": can't creat major device %d\n", dev->major);
		goto r_class;
	}

	/*device character add*/
	devno = MKDEV(dev->major ,0);

	mutex_init(&dev->mutex);
	cdev_init(&dev->cdev, &sensor_fops);

	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		pr_err(MODNAME ": registration fail %d\n", err);
		goto r_cdev;
	}

	return 0;

	/*Errors handling*/
r_class: 
	//unregister_chrdev_region(devno, 1);
	return -1;
r_cdev:
	unregister_chrdev_region(devno, 1);
	cdev_del(&dev->cdev);
	return -1;

}

static int sensor_remove(struct i2c_client *client) {
	struct sensor_device_desr *dev = i2c_get_clientdata(client);
	int devno;

	if (dev) {
		i2c_set_clientdata(client, NULL);
		cdev_del(&dev->cdev);
		devno = MKDEV(dev->major, 0);
		unregister_chrdev_region(devno, 1);
	}
	
	return 0;

}

static struct i2c_driver sensor_driver = {
	.driver  = {
		.name   = I2C_DRIVER_NAME,
		.owner  = THIS_MODULE,
	},
	.probe   = sensor_probe,
	.remove  = sensor_remove,
	.id_table= sensor_id,
};

/*-------------------- module init ------------------------------------------*/

static int __init sensor_init_module(void) {
	struct i2c_adapter *adapter = i2c_get_adapter(0);
	int ret = 0;

	printk(MODNAME ": entering module init \n");

	if (!adapter) {
		//pr_err(MODNAME ": Error getting i2c adapter\n");
		printk(MODNAME ": Error getting i2c adapter\n");
		ret = -ENODEV;
		goto exit1;
	}

	sensor_client = i2c_new_device(adapter, &sensor_i2c_board_info);
	if (!sensor_client) {
		//pr_err(MODNAME ": Error registering i2c device\n");
		printk(MODNAME ": Error registering i2c device\n");
		
		goto exit2;
	}

	ret = i2c_add_driver(&sensor_driver);
	if (ret < 0) {
		goto exit3;
	}

	i2c_put_adapter(adapter);


	printk(MODNAME ": module init ... successfully\n");

	return 0;

exit1: 
	return ret;
exit2: 
	i2c_put_adapter(adapter);
exit3: 
	i2c_unregister_device(sensor_client);
}

//module exit
static void __exit sensor_exit_module(void) {
	i2c_del_driver(&sensor_driver);
        i2c_unregister_device(sensor_client);
}

module_init(sensor_init_module);
module_exit(sensor_exit_module);

//module_i2c_driver(sensor_driver);
//
/*-----------------------device probe----------------------------------------*/

MODULE_AUTHOR("NGUYEN Qui");
MODULE_DESCRIPTION("I2C driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
