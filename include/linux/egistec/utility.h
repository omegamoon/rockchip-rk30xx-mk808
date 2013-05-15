#ifndef _MACROUTILITY_H_
#define _MACROUTILITY_H_

#define EGIS_SUCCESS(x) ((0==x) ? true : false)
#define EGIS_FAIL(x) 	((0!=x) ? true : false)


//========================== Debug Message ===================================//
//============================================================================//
#include <linux/kernel.h>

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define EgisMsg(switch, level, msg, ...) 				\
		if(switch){										\
			printk(level pr_fmt(msg), ##__VA_ARGS__);	\
		}

#if defined(DEBUG)
//#define pr_debug(fmt, ...) \
//    printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
//#define info(fmt, ...)\
//    printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
//#define VERBOSE 1
#endif

#endif 
