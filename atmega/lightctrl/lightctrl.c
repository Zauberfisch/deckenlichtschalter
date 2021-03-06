/*
 *  lightctrl - teensy firmware code
 *
 *
 *  Copyright (C) 2013 Bernhard Tittelbach <xro@realraum.at>
 *   uses avr-utils, anyio & co by Christian Pointner <equinox@spreadspace.org>
 *
 *  This file is part of lightctrl.
 *
 *  lightctrl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  lightctrl is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with lightctrl. If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"
#include "led.h"
#include "mypins.h"

#include "dualusbio.h"
#include "rf433.h"

uint8_t relais_state_ = 0;
///ENTPRELL_COUNT max is 2^8-1
#define ENTPRELL_COUNT 250
uint16_t buttons_pressed_ = 0;
uint16_t last_buttons_pressed_ = 0;
uint8_t button_entprell_counts_[NUM_BUTTONS];

void printStatus(void)
{
  printf("%c%c%c\n",relais_state_, (buttons_pressed_>>8) & 0xff, buttons_pressed_ & 0xff);
}

uint16_t entprellButton(uint8_t button_id, bool buttonstate)
{
  if (buttonstate)
  {
    if (button_entprell_counts_[button_id] < ENTPRELL_COUNT)
      button_entprell_counts_[button_id]++;
  } else {
    button_entprell_counts_[button_id]=0;
  }

  if (button_entprell_counts_[button_id] >= ENTPRELL_COUNT)
  {
    return 1<<button_id;
  } else {
    return 0;
  }
}

void readButtons(uint16_t *buttons)
{
  *buttons = 0;
  for (uint8_t c=0; c<6; c++)
  {
    if (TEST_BTN_ON(c))
    {
      *buttons |= entprellButton(c*2, true);
      //if on button is pressed, off button can't be pressed
      *buttons |= entprellButton(c*2+1, false);
    } else {
      *buttons |= entprellButton(c*2, false);
      //if on button is pressed, off button can't be pressed
      *buttons |= entprellButton(c*2+1, TEST_BTN_OFF(c));
    }
  }
  for (uint8_t c=0; c<3; c++)
  {
    *buttons |= entprellButton(c+12, TEST_BTN_SIG(c));
  }
}

void buttonsToNewState(void)
{
  if (buttons_pressed_ & (1<<12)) {
    relais_state_ = 0;
  } else if (buttons_pressed_ & (1<<13)) {
    relais_state_ = 0x0C;
  } else if (buttons_pressed_ & (1<<14)) {
    relais_state_ = RELAIS_MASK;
  }
  for (uint8_t c=0; c<6; c++)
  {
    if (buttons_pressed_ & (1<<(c*2)))
    {
      //on
      relais_state_ |= (1 << c);
    } else if (buttons_pressed_ & (1<<(c*2+1)))
    {
      //off
      relais_state_ &= ~(1 << c);
    }
  }
}

void applyRelaisState(uint8_t new_state)
{
  relais_state_ = new_state & RELAIS_MASK;
  RELAIS_PORT = (RELAIS_PORT & ~RELAIS_MASK) | (relais_state_ ^ RELAIS_INVERT_MASK);
}

typedef struct {
  uint8_t bufindex;
  uint8_t start_seen;
  uint8_t escape_seen;
} startescapereader;

//reads exactly bufflen chars
//does not start to fill buffer until start_escape char is seen
//further occurances of start_escape have to be escaped by an additional start_escape char
bool readFixedLenSeqIntoBufferWStartEscapeSymbol(uint8_t *buffer, startescapereader *pst, uint8_t buflen, uint8_t start_escape)
{
  int ReceivedByte=0;
  ReceivedByte = fgetc(usbio[0]);
  if (ReceivedByte == EOF)
    return false;

  if ((uint8_t) ReceivedByte == start_escape)
  {
    if (pst->escape_seen == 0)
    {
      pst->escape_seen=1;
      return false;
    }
  } else {
    if (pst->escape_seen)
    {
      pst->bufindex=0;
      pst->start_seen=1;
    }
  }
  pst->escape_seen=0;
  buffer[pst->bufindex] = (char) ReceivedByte;
  pst->bufindex++;
  pst->bufindex %= buflen;
  if (pst->start_seen && pst->bufindex == 0) {
    pst->start_seen = 0;
    return true;
  } else {
    return false;
  }
  //return (*bufindex == 0)
}

int main(void)
{
  uint8_t btns_on_pins[6] = {BTN_L1_ON_PIN,BTN_L2_ON_PIN,BTN_L3_ON_PIN,BTN_L4_ON_PIN,BTN_L5_ON_PIN,BTN_L6_ON_PIN};
  btns_on_pins_ = btns_on_pins;
  volatile uint8_t *btns_on_pinreg[6] = {&BTN_L1_ON_PORT-2,&BTN_L2_ON_PORT-2,&BTN_L3_ON_PORT-2,&BTN_L4_ON_PORT-2,&BTN_L5_ON_PORT-2,&BTN_L6_ON_PORT-2};
  btns_on_pinreg_ = btns_on_pinreg;
  uint8_t btns_off_pins[6] = {BTN_L1_OFF_PIN,BTN_L2_OFF_PIN,BTN_L3_OFF_PIN,BTN_L4_OFF_PIN,BTN_L5_OFF_PIN,BTN_L6_OFF_PIN};
  btns_off_pins_ = btns_off_pins;
  volatile uint8_t *btns_off_pinreg[6] = {&BTN_L1_OFF_PORT-2,&BTN_L2_OFF_PORT-2,&BTN_L3_OFF_PORT-2,&BTN_L4_OFF_PORT-2,&BTN_L5_OFF_PORT-2,&BTN_L6_OFF_PORT-2};
  btns_off_pinreg_ = btns_off_pinreg;
  uint8_t btns_sig_pins[3] = {BTN_C1_PIN,BTN_C2_PIN,BTN_C3_PIN};
  btns_sig_pins_ = btns_sig_pins;
  volatile uint8_t *btns_sig_pinreg[3] = {&BTN_C1_PORT-2,&BTN_C2_PORT-2,&BTN_C3_PORT-2};
  btns_sig_pinreg_ = btns_sig_pinreg;


  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  jtag_disable();

  cpu_init();
  led_init();
  dualusbio_init();
  dualusbio_make_stdio(1);
  sei();

  led_off();

  //set default all to INPUT
  DDRB = 0;
  DDRC = 0;
  DDRD = 0;
  DDRF = 0;
  //set pins for Relais to OUTPUT
  RELAIS_DDR |= RELAIS_MASK;
  //set pin for RF to OUTPUT
  PINMODE_OUTPUT(RF_DATA_OUT_DDR, RF_DATA_OUT_PIN);
  PIN_LOW(RF_DATA_OUT_PORT,RF_DATA_OUT_PIN);

  //set PULL-UP on button pins
  //PORTB = ~DDRB & PULLUP_DDRB;
  //PORTC = ~DDRC & PULLUP_DDRC;
  //PORTD = ~DDRD & PULLUP_DDRD;
  //PORTF = ~DDRF & PULLUP_DDRF;
  //set PULL-UP on everything that is not an OUTPUT to have well defined levels on unconnected and button pins
  PORTB = ~DDRB;
  PORTC = ~DDRC;
  PORTD = ~DDRD;
  PORTF = ~DDRF;

  applyRelaisState(0);

  startescapereader rf433_parseinfo;
  uint8_t rf433_send_buffer[4];
  memset((uint8_t*)rf433_send_buffer, 0, 4);
  memset((uint8_t*)&rf433_parseinfo, 0, sizeof(rf433_parseinfo));
  memset((uint8_t*)button_entprell_counts_, 0, sizeof(uint8_t)*NUM_BUTTONS);

  for(;;)
  {
    //read|send on ser2 for ceiling lights and relais state
    int16_t BytesReceived = dualusbio_bytes_received_std();
    //if we receive more than 16 states at once, it's propably an error
    if (BytesReceived > 0xf)
      BytesReceived = 0xf;
    while(BytesReceived > 0)
    {
      int ReceivedByte = fgetc(stdin);
      if (ReceivedByte != EOF && ReceivedByte <= 0xff)
      {
        applyRelaisState((uint8_t) ReceivedByte);
        printStatus();
      }
      BytesReceived--;
    }


    readButtons(&buttons_pressed_);

    if (last_buttons_pressed_ != buttons_pressed_ && buttons_pressed_ != 0)
    {
      buttonsToNewState();
      printStatus();
    }
    last_buttons_pressed_ = buttons_pressed_;

    // Read rf433 poweroutlet sequence to send
    BytesReceived = dualusbio_bytes_received(0);
    if (BytesReceived > 0xf)
      BytesReceived = 0xf;
    while(BytesReceived > 0)
    {
      BytesReceived--;
      if (readFixedLenSeqIntoBufferWStartEscapeSymbol(rf433_send_buffer,&rf433_parseinfo,3,(uint8_t)'>'))
      {
        // printf("rf: %x%x%x\n",rf433_send_buffer[0],rf433_send_buffer[1],rf433_send_buffer[2]);
        rf433_send_rf_cmd(rf433_send_buffer);
      }
    }

    dualusbio_task();
    led_toggle();
  }
}
