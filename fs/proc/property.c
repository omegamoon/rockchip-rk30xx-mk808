#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/rk_board_id.h>

#if defined(CONFIG_RK_BOARD_ID)
extern enum rk_board_id rk_get_board_id(void);

static char *kernel_property[BOARD_ID_NUM] = 
{
	"ril.function.dataonly=0\n" \
        "rild.libpath=/system/lib/libreference-ril-mt6229.so\n" \
        "rild.libargs=-d_/dev/ttyUSB2\n" \
        "ril.atchannel=/dev/ttyS1\n" \
        "ril.pppchannel=/dev/ttyACM0\n"\
        "ro.wac.service=0\n"\
        "ro.sf.hwrotation=0\n",

#if defined(CONFIG_BP_AUTO)
	"ril.function.dataonly=0\n" \
	"rild.libpath=/system/lib/libreference-ril-mu509.so\n" \
	"rild.libargs=-d_/dev/ttyUSB2\n" \
	"ril.atchannel=/dev/ttyS1\n" \
	"ril.pppchannel=/dev/ttyUSB244\n" \
	"ril.microphone=2\n" \
	"ril.loudspeaker=5\n" \
	"ril.switch.sound.path=0\n"\
	"ro.wac.service=1\n"\
  	"ro.sf.hwrotation=270\n",
#else
	"rild.libargs=-d_/dev/ttyUSB1\n" \
	"ril.pppchannel=/dev/ttyUSB2\n"  \
	"rild.libpath=/system/lib/libril-rk29-dataonly.so\n" \
	"ril.function.dataonly=1\n"\
	"ril.microphone=2\n" \
	"ril.loudspeaker=5\n" \
	"ril.switch.sound.path=0\n"\
	"ro.wac.service=1\n"\
	"ro.sf.hwrotation=270\n",
#endif

#if defined(CONFIG_BP_AUTO)  
	"ril.function.dataonly=0\n" \
	"rild.libpath=/system/lib/libreference-ril-mt6229.so\n" \
	"rild.libargs=-d_/dev/ttyUSB2\n" \
	"ril.atchannel=/dev/ttyS1\n" \
	"ril.pppchannel=/dev/ttyACM0\n"\
	"ro.wac.service=0\n"\
	"ro.sf.hwrotation=0\n",

#else	
	"rild.libargs=-d_/dev/ttyUSB1\n" \
	"ril.pppchannel=/dev/ttyUSB2\n"  \
	"rild.libpath=/system/lib/libril-rk29-dataonly.so\n" \
	"ril.function.dataonly=1\n"\
	"ro.wac.service=0\n"\
	"ro.sf.hwrotation=0\n",
#endif

#if defined(CONFIG_BP_AUTO)  
	"ril.function.dataonly=0\n" \
	"rild.libpath=/system/lib/libreference-ril-mt6229.so\n" \
	"rild.libargs=-d_/dev/ttyUSB2\n" \
	"ril.atchannel=/dev/ttyS1\n" \
	"ril.pppchannel=/dev/ttyACM0\n"\
	"ro.wac.service=0\n"\
	"ro.sf.hwrotation=0\n",
	/*
       "ril.function.dataonly=0\n" \
       "rild.libpath=/system/lib/libreference-ril-mt6229.so\n" \
       "rild.libargs=-d_/dev/ttyUSB244\n" \
       "ril.atchannel=/dev/ttyUSB244\n" \
       "ril.pppchannel=/dev/ttyACM0\n"\
       "ro.wac.service=0\n"\
       "ro.sf.hwrotation=0\n",
	*/
#else	
	"rild.libargs=-d_/dev/ttyUSB1\n" \
	"ril.pppchannel=/dev/ttyUSB2\n"  \
	"rild.libpath=/system/lib/libril-rk29-dataonly.so\n" \
	"ril.function.dataonly=1\n"\
	"ro.wac.service=0\n"\
	"ro.sf.hwrotation=0\n",
#endif

	"rild.libargs=-d_/dev/ttyUSB1\n" \
	"ril.pppchannel=/dev/ttyUSB2\n"  \
	"rild.libpath=/system/lib/libril-rk29-dataonly.so\n" \
	"ril.function.dataonly=1\n"\
	"ro.wac.service=0\n"\
	"ro.sf.hwrotation=0\n",

};

static int property_proc_show(struct seq_file *m, void *v)
{
	int id = -1;

	id = rk_get_board_id();

	if((id >= 0) && (sizeof(kernel_property[id]) > 0))
	seq_printf(m, "%s\n", kernel_property[id]);
	
	return 0;
}

static int property_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, property_proc_show, NULL);
}

static const struct file_operations property_proc_fops = {
	.open		= property_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static int __init proc_property_init(void)
{
	proc_create("kernel_property", 0, NULL, &property_proc_fops);
	return 0;
}
late_initcall(proc_property_init);
#endif
