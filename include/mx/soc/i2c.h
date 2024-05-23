
#ifndef __MX_SOC_I2C_H_
#define __MX_SOC_I2C_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>



enum i2c_status_e
{
    I2C_ERR_TIMEOUT = -6,
    I2C_ERR_SW_FAULT = -5,
    I2C_ERR_USAGE_FAULT = -4,
    I2C_ERR_ARBITRATION_LOST = -3,
    I2C_ERR_BUS_FAIL = -2,
    I2C_ERR_NACK = -1,

    I2C_TRANSFER_COMPLETE = 0,
    I2C_TRANSFER_PENDING,
};



struct i2c_drv;

typedef void (*i2c_callback_t)(int i2c_event);



void i2c_open(struct i2c_drv *drv, uint32_t speed, i2c_callback_t clbk);

bool i2c_bus_lockup_detected(struct i2c_drv *drv);
void i2c_resolve_bus_lockup(struct i2c_drv *drv);

void i2c_enable(struct i2c_drv *drv);
void i2c_disable(struct i2c_drv *drv);

int i2c_start_write_transfer(struct i2c_drv *drv, uint8_t addr, uint8_t *data, size_t data_len);
int i2c_start_read_transfer(struct i2c_drv *drv, uint8_t addr, uint8_t *cmd, size_t cmd_len, uint8_t *data, size_t data_len);
int i2c_continue_transfer(struct i2c_drv *drv);
int i2c_wait_for_transfer_end(struct i2c_drv *drv);



// Blocking functions

bool i2c_write(struct i2c_drv *drv, uint8_t addr, uint8_t *data, size_t data_len);
bool i2c_read(struct i2c_drv *drv, uint8_t addr, uint8_t *cmd, size_t cmd_len, uint8_t *data, size_t data_len);

bool i2c_read_regs(struct i2c_drv *drv, uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
bool i2c_write_regs(struct i2c_drv *drv, uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

bool i2c_read_reg(struct i2c_drv *drv, uint8_t addr, uint8_t reg, uint8_t *data);
bool i2c_write_reg(struct i2c_drv *drv, uint8_t addr, uint8_t reg, uint8_t data);



#endif /* __MX_SOC_I2C_H_ */
