
#include <mach/io.h>
#include <mach/board.h>
#include <mach/sram.h>
#include <asm/io.h>
#include <mach/cru.h>
#include <linux/regulator/rk29-pwm-regulator.h>

#define pwm_write_reg(id, addr, val)        __raw_writel(val, addr+(RK30_PWM01_BASE+(id>>1)*0x20000)+id*0x10)
#define pwm_read_reg(id, addr)              __raw_readl(addr+(RK30_PWM01_BASE+(id>>1)*0x20000+id*0x10))

#define cru_readl(offset)	readl_relaxed(RK30_CRU_BASE + offset)
#define cru_writel(v, offset)	do { writel_relaxed(v, RK30_CRU_BASE + offset); dsb(); } while (0)

static unsigned int __sramdata pwm_lrc,pwm_hrc;

#define LOOPS_PER_USEC	13
#define LOOP(loops) do { unsigned int i = loops; barrier(); while (--i) barrier(); } while (0)

#define grf_writel(v, offset)	do { writel_relaxed(v, RK30_GRF_BASE + offset); dsb(); } while (0)

#define gate_save_soc_clk(val,_save,cons,w_msk) \
	(_save)=cru_readl(cons);\
	cru_writel((((~(val)|(_save))&(w_msk))|((w_msk)<<16)),cons)

extern void  __sramfunc sram_printch(char byte);


#define GRF_GPIO0H_DIR_ADDR 0x0004 //方向
#define GRF_GPIO0H_DO_ADDR 0x003C  // 输出值
#define GRF_GPIO0H_EN_ADDR 0x0074  // 输出使能

#define GPIO0_PD7_DIR_OUT  0x80008000
#define GPIO0_PD7_DO_LOW  0x80000000
#define GPIO0_PD7_DO_HIGH  0x80008000
#define GPIO0_PD7_EN_MASK  0x80008000

static void __sramfunc rk29_pwm_set_core_voltage(unsigned int uV)
{
	u32 clk_gate2;
	char id = 3;
	//sram_printch('y');
#if 1
	gate_save_soc_clk(0
			  | (1 << CLK_GATE_ACLK_PEIRPH % 16)
			  | (1 << CLK_GATE_HCLK_PEIRPH % 16)
			  | (1 << CLK_GATE_PCLK_PEIRPH % 16)
			  , clk_gate2, CRU_CLKGATES_CON(2), 0
			  | (1 << ((CLK_GATE_ACLK_PEIRPH % 16) + 16))
			  | (1 << ((CLK_GATE_HCLK_PEIRPH % 16) + 16))
			  | (1 << ((CLK_GATE_PCLK_PEIRPH % 16) + 16)));
#endif

	/* iomux pwm3 */
	writel_relaxed((readl_relaxed(RK30_GRF_BASE + 0xB4) & ~(0x1<<14)) | (0x1<<14) |(0x1<<30), RK30_GRF_BASE + 0xB4);//PWM

	if (uV) {
		pwm_lrc = pwm_read_reg(id,PWM_REG_LRC);
		pwm_hrc = pwm_read_reg(id,PWM_REG_HRC);

	writel_relaxed((readl_relaxed(RK30_GRF_BASE + 0xB4) & ~(0x1<<14)) | (0x1<<30), RK30_GRF_BASE + 0xB4);//GPIO
	grf_writel(GPIO0_PD7_DIR_OUT, GRF_GPIO0H_DIR_ADDR);
	grf_writel(GPIO0_PD7_DO_HIGH, GRF_GPIO0H_DO_ADDR); 
	grf_writel(GPIO0_PD7_EN_MASK, GRF_GPIO0H_EN_ADDR);	
		
	}else
	{
	pwm_write_reg(id,PWM_REG_CTRL, PWM_DIV|PWM_RESET);
	pwm_write_reg(id,PWM_REG_LRC, pwm_lrc);
	pwm_write_reg(id,PWM_REG_HRC, pwm_hrc);
	
	pwm_write_reg(id,PWM_REG_CNTR, 0);
	pwm_write_reg(id,PWM_REG_CTRL, PWM_DIV|PWM_ENABLE|PWM_TimeEN);

	}

	LOOP(10 * 1000 * LOOPS_PER_USEC); /* delay 10ms */

	cru_writel(clk_gate2, CRU_CLKGATES_CON(2));
}

unsigned int __sramfunc rk29_suspend_voltage_set(unsigned int vol)
{
	
	rk29_pwm_set_core_voltage(1000000);
	return 0;

}
EXPORT_SYMBOL(rk29_suspend_voltage_set);
void __sramfunc rk29_suspend_voltage_resume(unsigned int vol)
{
	rk29_pwm_set_core_voltage(0);
}
EXPORT_SYMBOL(rk29_suspend_voltage_resume);