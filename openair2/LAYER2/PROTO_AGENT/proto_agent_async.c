/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file enb_agent_async.c
 * \brief channel implementation for async interface
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include "proto_agent_async.h"
#include "proto_agent_defs.h"

#include "log.h"


proto_agent_async_channel_t * proto_server_async_channel_info(mid_t mod_id, char *dst_ip, uint16_t dst_port) {

  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *) malloc(sizeof(proto_agent_channel_t));
  
  if (channel == NULL)
    goto error;

  channel->enb_id = mod_id;
  /*Create a socket*/
  printf("Starting async server\n");
  new_thread(new_link_server, (void *) &dst_port);
  channel->link = (void *) &dst_port;
  printf("Started async server\n");
  if (channel->link == NULL) goto error;
  
  LOG_I(PROTO_AGENT,"starting proto agent server for module id %d on ipv4 %s, port %d\n",  
	channel->enb_id,
	dst_ip,
	dst_port);
  
   /* 
   * create a message queue
   */ 
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;
  
   /* 
   * create a link manager 
   */  
  channel->manager = create_link_manager(channel->send_queue, channel->receive_queue, channel->link);
  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  LOG_I(PROTO_AGENT,"there was an error\n");
  return 1;
}


proto_agent_async_channel_t * proto_agent_async_channel_info(mid_t mod_id, char *dst_ip, uint16_t dst_port) {

  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *) malloc(sizeof(proto_agent_channel_t));
  
  if (channel == NULL)
    goto error;

  channel->enb_id = mod_id;
  /*Create a socket*/
  channel->link = new_link_client(dst_ip, dst_port);

  if (channel->link == NULL) goto error;
  

  LOG_I(PROTO_AGENT,"starting proto agent client for module id %d on ipv4 %s, port %d\n",  
	channel->enb_id,
	dst_ip,
	dst_port);
  
   /* 
   * create a message queue
   */ 
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;
  
   /* 
   * create a link manager 
   */  
  channel->manager = create_link_manager(channel->send_queue, channel->receive_queue, channel->link);
  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  LOG_I(PROTO_AGENT,"there was an error\n");
  return 1;
}

int proto_agent_async_msg_send(void *data, int size, int priority, void *channel_info) {
  proto_agent_async_channel_t *channel;
  channel = (proto_agent_channel_t *)channel_info;

  return message_put(channel->send_queue, data, size, priority);
}

int proto_agent_async_msg_recv(void **data, int *size, int *priority, void *channel_info) {
  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *)channel_info;

  return message_get(channel->receive_queue, data, size, priority);
}

void proto_agent_async_release(proto_agent_channel_t *channel) {
  proto_agent_async_channel_t *channel_info;
  channel_info = (proto_agent_async_channel_t *) channel->channel_info;

  destroy_link_manager(channel_info->manager);
  
  destroy_message_queue(channel_info->send_queue);
  destroy_message_queue(channel_info->receive_queue);
  
  close_link(channel_info->link);
  free(channel_info);
}