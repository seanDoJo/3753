#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage long sys_simple_add(int a, int b, int* c)
{
	printk(KERN_ALERT "numbers to add: %d + %d\n",a,b);
	*c = a + b;
	printk(KERN_ALERT "sum: %d\n",*c);
	return 0;
}
