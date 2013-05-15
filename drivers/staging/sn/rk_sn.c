#include <linux/syscalls.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/freezer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "../../mtd/rknand/api_flash.h"



static int rksn_debug = 0;
module_param(rksn_debug, int, S_IRUGO|S_IWUSR|S_IWGRP);

#define DBG(level, fmt, arg...) do {			\
	if (rksn_debug >= level) 					\
	    printk("rk_sn_init:" fmt , ## arg); } while (0)

#define  BLUE_MAC "/data/blue_mac"
#define  SN_FILE  "/data/sn"

struct rockchip_sn{
	struct timer_list sn_timer;
	struct work_struct sn_work;
};

 static struct kobject android_sn_kobj;
 static char sn_buf[32] = {0};
//static DEVICE_ATTR(snvalue, 0444, show_sn_value, NULL);
// static DEVICE_ATTR(vendor, 0444, show_sn_value, NULL);

spinlock_t sn_spin;

 static ssize_t  show_sn_value(struct kobject *kobject, struct attribute *attr,char *buf)
 {
	 ssize_t ret = 0;


	printk("%s.%s.strlen(sn_buf)=%d.enter\n",__FUNCTION__,sn_buf,strlen(sn_buf));
    
	if(strlen(sn_buf) > 3)
		 ret = sprintf(buf, "%s\n", sn_buf);
	else
	{
			printk("%s...error\n",__FUNCTION__);
	ret = sprintf(buf,"%s\n","yangjierockchip");
	}
	printk("%s.......ret=%d\n",__FUNCTION__,ret);
	  return  ret+1;

}

static void  sys_get_value(void)
{

	 long fd = 0;

   	fd = sys_open("/sys/devices/platform/rk30_i2c.0/i2c-0/name",O_RDONLY,0);
     if (fd < 0)
	{
		printk("%s..%d.. get attr error \n",__FUNCTION__,fd);
	}
	else
	{
		printk("%s....open  success\n",__FUNCTION__);	
	}
		
	spin_lock(&sn_spin);
	sys_read(fd,sn_buf,32);
	printk("%s....sn_buf=%s\n",__FUNCTION__,sn_buf);
	spin_unlock(&sn_spin);
	sys_close(fd);
}


struct attribute sn_attr = {
	        .name = "snvalue",
			        .mode = S_IRWXUGO,
};
static struct attribute *def_attrs[] = {
		        &sn_attr,
				        NULL,
};
 
  
  struct sysfs_ops obj_test_sysops =
  {
		          .show = show_sn_value,
				          .store = NULL,
 };

struct kobj_type ktype = 
{
		        .release = NULL,
				        .sysfs_ops=&obj_test_sysops,
						        .default_attrs=def_attrs,
};


static int sn_sys_init(void)
{
	int ret =0;
#if 0

	android_sn_kobj =  kobject_create_and_add("sn_kobj", NULL);

	if (android_sn_kobj == NULL)
	{

		printk("android_sn_kobj  %s.. error\n",__FUNCTION__);
		ret = -ENOMEM;
		goto err;
		
	}

	ret = sysfs_create_file(android_sn_kobj, &dev_attr_vendor.attr);
	if (ret)
	{
		
		printk("dev_attr_vendor  %s.. error\n",__FUNCTION__);
		goto err1;
		
	}

err1:
	 kobject_del(android_sn_kobj);
err:
	 return ret ;
#endif
	 printk("%s...enter\n",__FUNCTION__);
	kobject_init_and_add(&android_sn_kobj,&ktype,NULL,"eric_test");
	return 0; 

}

static void SN_get(struct work_struct *work)
{

	char *pbuf = (char *)kzalloc(512, GFP_KERNEL);;
	char SN[31] = {0};
	char  bd_addr[6] = {0};
	int  i = 0;

//	unsigned short *pSnLen = NULL;
	long fd = 0;
	char buf_tmp[15] = {0};
	char mac_tmp[30] = {0};

	DBG(1,"%s........enter\n",__FUNCTION__);
	GetSNSectorInfo(pbuf);
	unsigned short *pSnLen = (unsigned short *)pbuf; 
	memcpy(SN,pbuf+2,*pSnLen);	
	//memcpy(sn_buf,pbuf+2,*pSnLen);	
	//printk("%s..psnlen=%d...SN=%s\n",__FUNCTION__,*pSnLen,SN);

    	fd = sys_open(SN_FILE,O_CREAT | O_RDWR,S_IRWXU|S_IRGRP|S_IROTH);
    
	if(fd < 0)
    	{
		printk("%s SN_get: open file /data/sn failed\n",__FILE__);
		return;
	}
	sys_write(fd, (const char __user *)SN, *pSnLen);
	
    sys_close(fd);
		fd = sys_open(BLUE_MAC,O_CREAT | O_RDWR,S_IRWXU|S_IRGRP|S_IROTH);
    

	if(fd < 0)
    	{
		printk("%s SN_get: open file /data/blue_mac failed\n",__FILE__);
		return;
	}
            for(i=499; i<=504; i++)
			 {
				//printk("%s....*(pbuf+%d)=%x\n",__FUNCTION__,i,*(pbuf+i));
				bd_addr[504-i] = *(pbuf+i);
			//sprintf(mac_tmp+i-499+4,"%02x:",*(pbuf+i));
			//	printk("%s.....bd_addr[504-i] =%x\n",__FUNCTION__,bd_addr[504-i]);
			}
		sprintf(mac_tmp,"%02x:%02x:%02x:%02x:%02x:%02x",bd_addr[5],bd_addr[4],bd_addr[3],bd_addr[2],bd_addr[1],bd_addr[0]);
		//printk("%s....msc_tmp=%s\n",__FUNCTION__,mac_tmp);
	sys_write(fd, (const char __user *)mac_tmp,strlen(mac_tmp)); 
	
    sys_close(fd);
	kfree(pbuf);
	/*
		fd = sys_open(BLUE_MAC,O_CREAT | O_RDWR,S_IRWXU|S_IRGRP|S_IROTH);
		sys_read(fd,buf_tmp,15);
		for (i=0;i<15;i++)
				printk("buf_tmp[i]=%4x",buf_tmp[i]);
				printk("mac over\n");
		sys_close(fd);
	*/
	DBG(1,"%s.......blue over.\n",__FUNCTION__);	
}


static void SN_get_timer(unsigned long data)
{
	struct rockchip_sn *rk_sn = (struct rockchip_sn *)data;  
	schedule_work(&rk_sn->sn_work);	
}
static int __init rk_sn_init(void)
{
	int ret = 0;
	struct rockchip_sn *rk_sn; 

	rk_sn = kzalloc(sizeof(*rk_sn), GFP_KERNEL);
	if (!rk_sn) {
		printk("failed to allocate device info data in rk_sn_init\n");
		ret = -ENOMEM;
		return ret;
	}
	INIT_WORK(&rk_sn->sn_work,SN_get);
	setup_timer(&rk_sn->sn_timer, SN_get_timer, (unsigned long)rk_sn);
	rk_sn->sn_timer.expires  = jiffies + 1000;
	add_timer(&rk_sn->sn_timer);
	//spin_lock_init(&sn_spin);   
	
//sys_get_value();
	//sn_sys_init();
	printk("%s.rksn_debug=%d..ok\n",__FUNCTION__,rksn_debug);
	return 0;

}
 
module_init(rk_sn_init);

