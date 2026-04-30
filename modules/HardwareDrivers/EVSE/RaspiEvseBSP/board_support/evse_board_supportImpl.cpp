// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "evse_board_supportImpl.hpp"

#include <chrono>

namespace module {
namespace board_support {

void evse_board_supportImpl::init() {
}

void evse_board_supportImpl::ready() {
    i2c_read_thread_handle = std::thread(&evse_board_supportImpl::i2c_read_thread, this);
}

void evse_board_supportImpl::handle_enable(bool& value) {
    // your code for cmd enable goes here
    EVLOG_info << "evse_board_supportImpl::handle_enable " << value;
}

void evse_board_supportImpl::handle_pwm_on(double& value) {
    // your code for cmd pwm_on goes here
    EVLOG_info << "evse_board_supportImpl::handle_pwm_on " << value;
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

        types::board_support_common::BspEvent event;
        event.event = types::board_support_common::Event::A;
        publish_event(event);
    }
}

} // namespace board_support
} // namespace module
