#ifndef _BH1750_H
#define _BH1750_H

#include <rthw.h>
#include <rtthread.h>

/*bh1750 device address */
#define BH1750_ADDR 0x23

/*bh1750 registers define */
#define BH1750_POWER_DOWN   	0x00	// power down
#define BH1750_POWER_ON			0x01	// power on
#define BH1750_RESET			0x07	// reset	
#define BH1750_CON_H_RES_MODE	0x10	// Continuously H-Resolution Mode
#define BH1750_CON_H_RES_MODE2	0x11	// Continuously H-Resolution Mode2 
#define BH1750_CON_L_RES_MODE	0x13	// Continuously L-Resolution Mode
#define BH1750_ONE_H_RES_MODE	0x20	// One Time H-Resolution Mode
#define BH1750_ONE_H_RES_MODE2	0x21	// One Time H-Resolution Mode2
#define BH1750_ONE_L_RES_MODE	0x23	// One Time L-Resolution Mode

int bh1750_power_down();
int bh1750_power_on();
int bh1750_init();
float bh1750_read_light();

#endif
