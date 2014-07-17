/*
 * Copyright 2014 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/interrupt.h>
#include <avr/io.h>

#include <debug.h>
#include <octoclock.h>
#include <clkdist.h>
#include <state.h>

void led(LEDs which, int turn_it_on) {

    // selects the proper bit
    uint8_t LED = 0x20 << which;

    if(turn_it_on)
        PORTC |= LED;
    else
        PORTC &= ~LED;
}

void LEDs_Off(void){
    led(Top,false);
    led(Middle,false);
    led(Bottom,false);
}

void force_internal(void){
    led(Top,true);
    led(Middle,false);
    led(Bottom,true);

    setup_TI_CDCE18005(Primary_GPS);

    // Set PPS to Primary (1) n.b.:  "1" in general means "Internal" for all
    // such signals
    PORTA |= (1<<PA6);
}

void force_external(void){
    led(Top, false);
    led(Middle, true);
    led(Bottom, true);

    setup_TI_CDCE18005(Secondary_Ext);

    // Set PPS to External
    PORTA &= ~(1<<PA6);
}

void prefer_internal(void){
    // if internal is NOT OK, then force external
    if(global_gps_present)
        force_internal();
    else if(global_ext_ref_is_present)
        force_external();
    else
        LEDs_Off();
}

void prefer_external(void){
    // if external is NOT OK, then force internal
    if(global_ext_ref_is_present)
        force_external();
    else if(global_gps_present)
        force_internal();
    else
        LEDs_Off();
}

bool is_ext_ref_present(void){
    volatile uint8_t prev = (PINE & (1<<DDE7));
    volatile uint8_t now;

    for(uint16_t i = 1; i < 512; i++){
        now = (PINE & (1<<DDE7));
        if(prev != now) return true;
    }

    return false;
}

bool is_gps_present(void){
    return (PIND & (1<<DDD4));
}

ref_t which_ref(void){
    if(!global_gps_present && !global_ext_ref_is_present) global_which_ref = NO_REF;
    else if(global_gps_present && !global_ext_ref_is_present) global_which_ref = INTERNAL;
    else if(!global_gps_present && global_ext_ref_is_present) global_which_ref = EXTERNAL;
    else global_which_ref = (get_switch_pos() == UP) ? INTERNAL : EXTERNAL;

    return global_which_ref;
}

void check_what_is_present(void){
    global_ext_ref_is_present = is_ext_ref_present();
    global_gps_present = is_gps_present();
}

switch_pos_t get_switch_pos(void){
    uint8_t portC = PINC;

    // UP is prefer internal,
    // DOWN is prefer external
    return (portC & (1<<DDC1)) ? DOWN : UP;
}
