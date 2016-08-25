/*
 * Copyright (c) 2016, Swiss Federal Institute of Technology (ETH Zurich).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author:  Reto Da Forno
 */

/**
 * @brief Low-Power Wireless Bus Test Application
 *
 * A simple range test application. Each source node sends some status data
 * (RSSI, battery voltage, temperature, ...) to the host in each round.
 */


#include "main.h"

uint8_t bolt_buffer[BOLT_CONF_MAX_MSG_LEN];  /* tmp buffer to read from BOLT */
/*---------------------------------------------------------------------------*/
void
send_msg(uint16_t recipient,
         const uint8_t* data,
         uint8_t len,
         message_type_t type)
{
#define LWB_STREAM_ID_STATUS_MSG        1

  /* TODO use different stream IDs for different message types */

  static uint16_t seq_no = 0;

  message_t msg;
  msg.header.device_id   = node_id;
  msg.header.type        = type;
  msg.header.payload_len = len;
  msg.header.seqnr       = seq_no++;
  MSG_SET_CRC16(&msg, crc16(data, len, 0));

  if(data) {
    memcpy(msg.payload, data, len);
  }
  if(!lwb_send_pkt(recipient, LWB_STREAM_ID_STATUS_MSG,
                   (uint8_t*)&msg, msg.header.payload_len + MSG_HDR_LEN)) {
    DEBUG_PRINT_WARNING("message dropped (queue full)");
  } else {
    DEBUG_PRINT_INFO("message for node %u added to TX queue", recipient);
  }
}
/*---------------------------------------------------------------------------*/
void
host_init(void)
{
#if LWB_CONF_USE_LF_FOR_WAKEUP
  SVS_DISABLE;
#endif /* LWB_CONF_USE_LF_FOR_WAKEUP */

  DEBUG_PRINT_MSG_NOW("crc: 0x%04x", crc16((uint8_t*)"hello world!", 12, 0));
}
/*---------------------------------------------------------------------------*/
void
host_run(void)
{
  /* for BOLT messages */
  message_t bolt_msg;

  /* analyze and print the received data */
  uint8_t pkt_len = 0;
  message_t msg;
  do {
    if(lwb_rcv_pkt((uint8_t*)&msg, 0, 0)) {
      /* use DEBUG_PRINT_MSG_NOW to prevent a queue overflow */
      DEBUG_PRINT_MSG_NOW("data packet received from node %u (timestamp: %lu)",
                          msg.header.device_id, msg.cc430_health.lfxt_ticks);
      /* forward the packet: write it to BOLT */
      BOLT_WRITE((uint8_t*)&msg, msg.header.payload_len + MSG_HDR_LEN);
    }
  } while(pkt_len);


  /* handle timestamp requests */
  uint64_t time_last_req = bolt_handle_timereq();
  if(time_last_req) {
    /* write the timestamp to BOLT (convert to us) */
    bolt_msg.header.type = MSG_TYPE_TIMESTAMP;
    bolt_msg.timestamp = time_last_req * 1000000 / ACLK_SPEED + LWB_CLOCK_OFS;
    BOLT_WRITE((uint8_t*)&bolt_msg, MSG_HDR_LEN + sizeof(timestamp_t));
  }
  /* msg available from BOLT? */
  uint16_t msg_cnt = 0;
  while(BOLT_DATA_AVAILABLE) {
    uint8_t msg_len = 0;
    BOLT_READ(bolt_buffer, msg_len);
    if(msg_len) {
      msg_cnt++;
      memcpy(&bolt_msg, bolt_buffer, MSG_PKT_LEN);
      if(bolt_msg.header.type == MSG_TYPE_LWB_CMD) {
        if(bolt_msg.lwb_cmd.type == LWB_CMD_SET_SCHED_PERIOD) {
          /* adjust the period */
          lwb_sched_set_period(bolt_msg.lwb_cmd.value);
          DEBUG_PRINT_INFO("LWB period set to %us", bolt_msg.lwb_cmd.value);
        } else if(bolt_msg.lwb_cmd.type == LWB_CMD_SET_STATUS_PERIOD) {
          /* broadcast the message */
          send_msg(bolt_msg.lwb_cmd.target_id, bolt_msg.payload,
                   sizeof(lwb_cmd_t), MSG_TYPE_LWB_CMD);
        } else if(bolt_msg.lwb_cmd.type == LWB_CMD_PAUSE) {
          /* stop */
          while(BOLT_DATA_AVAILABLE) {        /* flush the queue */
            BOLT_READ(bolt_buffer, msg_len);
          }
          /* configure a port interrupt for the IND pin */
          __dint();
          PIN_IES_RISING(BOLT_CONF_IND_PIN);
          PIN_CLR_IFG(BOLT_CONF_IND_PIN);
          PIN_INT_EN(BOLT_CONF_IND_PIN);
          if(BOLT_DATA_AVAILABLE) {
            DEBUG_PRINT_MSG_NOW("failed to stop LWB");
            __eint();
          } else {
            lwb_pause();
            DEBUG_PRINT_MSG_NOW("LWB paused");
            __bis_status_register(GIE | SCG0 | SCG1 | CPUOFF);
            __no_operation();
            DEBUG_PRINT_MSG_NOW("LWB resumed");
            lwb_resume();
            continue;
          }
        }
      }
    }
  }
  if(msg_cnt) {
    DEBUG_PRINT_MSG_NOW("%u messages read from BOLT", msg_cnt);
  }
}
/*---------------------------------------------------------------------------*/
#if !DEBUG_PORT2_INT
ISR(PORT2, port2_interrupt)
{
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  if(PIN_IFG(BOLT_CONF_IND_PIN)) {
    while(BOLT_DATA_AVAILABLE) {
      uint8_t msg_len = 0;
      BOLT_READ(bolt_buffer, msg_len);
      message_t* msg = (message_t*)bolt_buffer;
      if(msg_len && msg->header.type == MSG_TYPE_LWB_CMD &&
         msg->lwb_cmd.type == LWB_CMD_RESUME) {
        PIN_INT_OFF(BOLT_CONF_IND_PIN);
        __bic_status_register_on_exit(SCG0 | SCG1 | CPUOFF);
        break;
      }
    }
    PIN_CLR_IFG(BOLT_CONF_IND_PIN);
  }

  ENERGEST_OFF(ENERGEST_TYPE_CPU);
}
#endif /* DEBUG_PORT2_INT */
/*---------------------------------------------------------------------------*/
