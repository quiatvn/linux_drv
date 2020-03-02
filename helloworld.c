#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

static char *mystr = "xin chao";
static int myint = 1;
static int myarr[3] = {1 ,2, 3};

module_param(mystr, charp, S_IRUGO);
module_param(myint, int, S_IRUGO);
module_param_array(myarr, int, NULL, S_IWUSR | S_IRUSR);

/*
static int foo() {
	pr_info("my string is a string: %s\n", mystr);
	return myint;
}
*/

static int __init helloworld_init(void) {
	pr_info("Hello world %s!\n", mystr);
	return 0;
}

static void __exit helloworld_exit(void) {
	pr_info("End of programme!\n");
}

module_init(helloworld_init);
module_exit(helloworld_exit);
MODULE_PARM_DESC(myint, "this is my int");
MODULE_PARM_DESC(mystr, "this is my string");
MODULE_PARM_DESC(myarr, "this is my aray");
MODULE_AUTHOR("Qui");
MODULE_LICENSE("GPL");
