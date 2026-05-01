// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "evse_board_supportImpl.hpp"

#include <chrono>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>


/**
 * i2c_open() - Open an I2C bus and bind it to a slave address.
 *
 * @bus_path:  Path to the I2C bus device, e.g. "/dev/i2c-1"
 * @addr:      7-bit I2C slave address
 *
 * Returns a file descriptor >= 0 on success, -1 on failure.
 */
static int i2c_open(const char *bus_path, uint8_t addr)
{
    int fd = open(bus_path, O_RDWR);
    if (fd < 0) {
        perror("i2c_open: open");
        return -1;
    }
 
    /* Tell the kernel which slave device we want to talk to */
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("i2c_open: ioctl I2C_SLAVE");
        close(fd);
        return -1;
    }
 
    return fd;
}

/**
 * Perform an atomic write+read using the ioctl interface.
 *
 * Some devices require the write and read to be a single I2C transaction
 * (no STOP condition between them — a "repeated START"). The I2C_RDWR ioctl
 * achieves this when plain write()+read() sequences don't work.
 *
 * @fd:       Open I2C file descriptor
 * @addr:     7-bit slave address
 * @reg:      Register to read from
 * @buf:      Buffer to store result
 * @len:      Number of bytes to read
 *
 * Returns 0 on success, -1 on failure.
 */
static int i2c_read_write(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data data;
 
    /* Message 0: write the register address */
    msgs[0].addr  = addr;
    msgs[0].flags = 0;          /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;
 
    /* Message 1: read the data (repeated START, no STOP between) */
    msgs[1].addr  = addr;
    msgs[1].flags = I2C_M_RD;  /* read */
    msgs[1].len   = len;
    msgs[1].buf   = buf;
 
    data.msgs  = msgs;
    data.nmsgs = 2;
 
    if (ioctl(fd, I2C_RDWR, &data) < 0) {
        perror("i2c_read_write: ioctl I2C_RDWR");
        return -1;
    }
    return 0;
}

/**
 * Perform an atomic write+write using the ioctl interface.
 *
 * Some devices require the write and write to be a single I2C transaction
 * (no STOP condition between them — a "repeated START"). The I2C_RDWR ioctl
 * achieves this when plain write()+write() sequences don't work.
 *
 * @fd:       Open I2C file descriptor
 * @addr:     7-bit slave address
 * @reg:      Register to write to
 * @buf:      Buffer to store result
 * @len:      Number of bytes to write
 *
 * Returns 0 on success, -1 on failure.
 */
static int i2c_write_write(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data data;
 
    /* Message 0: write the register address */
    msgs[0].addr  = addr;
    msgs[0].flags = 0;          /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;
 
    /* Message 1: read the data (repeated START, no STOP between) */
    msgs[1].addr  = addr;
    msgs[1].flags = 0;  /* write */
    msgs[1].len   = len;
    msgs[1].buf   = buf;
 
    data.msgs  = msgs;
    data.nmsgs = 2;
 
    if (ioctl(fd, I2C_RDWR, &data) < 0) {
        perror("i2c_read_write: ioctl I2C_RDWR");
        return -1;
    }
    return 0;
}

namespace module {
namespace board_support {

void evse_board_supportImpl::init() {
    
    fd = i2c_open(mod->config.i2c_dev.c_str(), mod->config.i2c_addr);
    if(fd == -1){
        EVLOG_error << "evse_board_supportImpl::init:  i2c_open() failed: " << fd; 
        return;
    }
    else{
        EVLOG_info << "evse_board_supportImpl::init:  i2c_open() success: " << fd; 
    }

    uint8_t sw_type;
    uint8_t sw_ver;

    if(i2c_read_write(fd, mod->config.i2c_addr, 0, &sw_type, 1) != 0)
    {
        EVLOG_error << "evse_board_supportImpl::init:  i2c_read_write() failed"; 
    }

    if(i2c_read_write(fd, mod->config.i2c_addr, 1, &sw_ver, 1) != 0)
    {
        EVLOG_error << "evse_board_supportImpl::init:  i2c_read_write() failed"; 
    }

    EVLOG_info << "sw_ver: " << (int)sw_ver;
    EVLOG_info << "sw_type: " << (int)sw_type;
}

void evse_board_supportImpl::ready() {
    i2c_read_thread_handle = std::thread(&evse_board_supportImpl::i2c_read_thread, this);
}

void evse_board_supportImpl::handle_enable(bool& value) {
    // your code for cmd enable goes here
    EVLOG_info << "evse_board_supportImpl::handle_enable " << value;

    if(value == true)
    {
        uint8_t pwm_int = pwm;
        if(i2c_write_write(fd, mod->config.i2c_addr, 6, &pwm_int, 1) != 0){
            EVLOG_error << "evse_board_supportImpl::handle_enable:  i2c_write_write() failed"; 
        }

    }
    else
    {
        uint8_t pwm_int = 0;
        if(i2c_write_write(fd, mod->config.i2c_addr, 6, &pwm_int, 1) != 0){
            EVLOG_error << "evse_board_supportImpl::handle_enable:  i2c_write_write() failed"; 
        }
    }
}

void evse_board_supportImpl::handle_pwm_on(double& value) {
    // your code for cmd pwm_on goes here
    EVLOG_info << "evse_board_supportImpl::handle_pwm_on " << value;

    pwm = value;

    uint8_t pwm_int = pwm;
    if(i2c_write_write(fd, mod->config.i2c_addr, 6, &pwm_int, 1) != 0){
        EVLOG_error << "evse_board_supportImpl::handle_enable:  i2c_write_write() failed"; 
    }
}

void evse_board_supportImpl::handle_cp_state_X1() {
    // your code for cmd cp_state_X1 goes here
    EVLOG_info << "evse_board_supportImpl::handle_cp_state_X1()";
}

void evse_board_supportImpl::handle_cp_state_F() {
    // your code for cmd cp_state_F goes here
    EVLOG_info << "evse_board_supportImpl::handle_cp_state_F()";
}

void evse_board_supportImpl::handle_cp_state_E() {
    // your code for cmd cp_state_E goes here
    EVLOG_info << "evse_board_supportImpl::handle_cp_state_E()";
}

void evse_board_supportImpl::handle_allow_power_on(types::evse_board_support::PowerOnOff& value) {
    // your code for cmd allow_power_on goes here
    EVLOG_info << "evse_board_supportImpl::handle_allow_power_on";
}

void evse_board_supportImpl::handle_ac_switch_three_phases_while_charging(bool& value) {
    // your code for cmd ac_switch_three_phases_while_charging goes here
    EVLOG_info << "evse_board_supportImpl::handle_ac_switch_three_phases_while_charging " << value;
}

void evse_board_supportImpl::handle_ac_set_overcurrent_limit_A(double& value) {
    // your code for cmd ac_set_overcurrent_limit_A goes here
    EVLOG_info << "evse_board_supportImpl::handle_ac_set_overcurrent_limit_A " << value;
}

void evse_board_supportImpl::i2c_read_thread(){
    
    for(;;)
    {
        sleep(1);

        uint16_t adc_pos;
        uint16_t adc_neg;

        if(i2c_read_write(fd, mod->config.i2c_addr, 2, (uint8_t*)&adc_pos, 2) != 0)
        {
            EVLOG_error << "evse_board_supportImpl::init:  i2c_read_write(adc_pos) failed"; 
        }

        if(i2c_read_write(fd, mod->config.i2c_addr, 4, (uint8_t*)&adc_neg, 2) != 0)
        {
            EVLOG_error << "evse_board_supportImpl::init:  i2c_read_write(adc_neg) failed"; 
        }

        double cp_pos_v = ntohs(adc_pos)*13.2/1024;
        double cp_neg_v = ntohs(adc_neg)*13.2/1024;

        EVLOG_info << "CP: (" << adc_pos << ") " << cp_pos_v << " (" << adc_neg << ") " << cp_neg_v;

        types::board_support_common::BspEvent event;
        event.event = types::board_support_common::Event::A;
        publish_event(event);
    }
}

} // namespace board_support
} // namespace module
