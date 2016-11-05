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

/*! \file flexran_agent.h
 * \brief top level flexran agent receive thread and itti task
 * \author Xenofon Foukas and Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#include "flexran_agent_common.h"
#include "log.h"
#include "flexran_agent.h"
#include "flexran_agent_mac_defs.h"

#include "flexran_agent_extern.h"

#include "assertions.h"

#include "flexran_agent_net_comm.h"
#include "flexran_agent_async.h"

//#define TEST_TIMER

flexran_agent_instance_t flexran_agent[NUM_MAX_ENB];

char in_ip[40];
static uint16_t in_port;
char local_cache[40];

void *send_thread(void *args);
void *receive_thread(void *args);
pthread_t new_thread(void *(*f)(void *), void *b);
Protocol__FlexranMessage *flexran_agent_timeout(void* args);


int agent_task_created = 0;
/* 
 * enb agent task mainly wakes up the tx thread for periodic and oneshot messages to the controller 
 * and can interact with other itti tasks
*/
void *flexran_agent_task(void *args){

  //flexran_agent_instance_t         *d = (flexran_agent_instance_t *) args;
  Protocol__FlexranMessage *msg;
  void *data;
  int size;
  err_code_t err_code;
  int                   priority;

  MessageDef                     *msg_p           = NULL;
  const char                     *msg_name        = NULL;
  instance_t                      instance;
  int                             result;
  struct flexran_agent_timer_element_s * elem = NULL;

  itti_mark_task_ready(TASK_FLEXRAN_AGENT);

  do {
    // Wait for a message
    itti_receive_msg (TASK_FLEXRAN_AGENT, &msg_p);
    DevAssert(msg_p != NULL);
    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(FLEXRAN_AGENT, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;
    
    case TIMER_HAS_EXPIRED:
      msg = flexran_agent_process_timeout(msg_p->ittiMsg.timer_has_expired.timer_id, msg_p->ittiMsg.timer_has_expired.arg);
      if (msg != NULL){
	data=flexran_agent_pack_message(msg,&size);
	elem = get_timer_entry(msg_p->ittiMsg.timer_has_expired.timer_id);
	if (flexran_agent_msg_send(elem->agent_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
	  err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
	  goto error;
	}

	LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
      }
      break;

    default:
      LOG_E(FLEXRAN_AGENT, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    continue;
  error:
    LOG_E(FLEXRAN_AGENT,"flexran_agent_task: error %d occured\n",err_code);
  } while (1);

  return NULL;
}

void *receive_thread(void *args) {

  flexran_agent_instance_t         *d = args;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexranMessage *msg;
  
  while (1) {

    while (flexran_agent_msg_recv(d->enb_id, FLEXRAN_AGENT_DEFAULT, &data, &size, &priority) == 0) {
      
      LOG_D(FLEXRAN_AGENT,"received message with size %d\n", size);
  
      // Invoke the message handler
      msg=flexran_agent_handle_message(d->enb_id, data, size);

      free(data);
    
      // check if there is something to send back to the controller
      if (msg != NULL){
	data=flexran_agent_pack_message(msg,&size);

	if (flexran_agent_msg_send(d->enb_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
	  err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
	  goto error;
	}
      
	LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
      } 
    }
  }
    
  return NULL;

error:
  LOG_E(FLEXRAN_AGENT,"receive_thread: error %d occured\n",err_code);
  return NULL;
}


/* utility function to create a thread */
pthread_t new_thread(void *(*f)(void *), void *b) {
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att)){ 
    fprintf(stderr, "pthread_attr_init err\n"); 
    exit(1); 
  }

  struct sched_param sched_param_recv_thread;

  sched_param_recv_thread.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
  pthread_attr_setschedparam(&att, &sched_param_recv_thread);
  pthread_attr_setschedpolicy(&att, SCHED_FIFO);

  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED)) { 
    fprintf(stderr, "pthread_attr_setdetachstate err\n"); 
    exit(1); 
  }
  if (pthread_create(&t, &att, f, b)) { 
    fprintf(stderr, "pthread_create err\n"); 
    exit(1); 
  }
  if (pthread_attr_destroy(&att)) { 
    fprintf(stderr, "pthread_attr_destroy err\n"); 
    exit(1); 
  }

  return t;
}

int channel_container_init = 0;
int flexran_agent_start(mid_t mod_id, const Enb_properties_array_t* enb_properties){
  
  int channel_id;
  
  flexran_set_enb_vars(mod_id, RAN_LTE_OAI);
  flexran_agent[mod_id].enb_id = mod_id;
  
  /* 
   * check the configuration
   */ 
  if (enb_properties->properties[mod_id]->flexran_agent_cache != NULL) {
    strncpy(local_cache, enb_properties->properties[mod_id]->flexran_agent_cache, sizeof(local_cache));
    local_cache[sizeof(local_cache) - 1] = 0;
  } else {
    strcpy(local_cache, DEFAULT_FLEXRAN_AGENT_CACHE);
  }
  
  if (enb_properties->properties[mod_id]->flexran_agent_ipv4_address != NULL) {
    strncpy(in_ip, enb_properties->properties[mod_id]->flexran_agent_ipv4_address, sizeof(in_ip) );
    in_ip[sizeof(in_ip) - 1] = 0; // terminate string
  } else {
    strcpy(in_ip, DEFAULT_FLEXRAN_AGENT_IPv4_ADDRESS ); 
  }
  
  if (enb_properties->properties[mod_id]->flexran_agent_port != 0 ) {
    in_port = enb_properties->properties[mod_id]->flexran_agent_port;
  } else {
    in_port = DEFAULT_FLEXRAN_AGENT_PORT ;
  }
  LOG_I(FLEXRAN_AGENT,"starting enb agent client for module id %d on ipv4 %s, port %d\n",  
	flexran_agent[mod_id].enb_id,
	in_ip,
	in_port);

  /*
   * Initialize the channel container
   */
  if (!channel_container_init) {
    flexran_agent_init_channel_container();
    channel_container_init = 1;
  }
  /*Create the async channel info*/
  flexran_agent_instance_t *channel_info = flexran_agent_async_channel_info(mod_id, in_ip, in_port);

  /*Create a channel using the async channel info*/
  channel_id = flexran_agent_create_channel((void *) channel_info, 
					flexran_agent_async_msg_send, 
					flexran_agent_async_msg_recv,
					flexran_agent_async_release);

  
  if (channel_id <= 0) {
    goto error;
  }

  flexran_agent_channel_t *channel = get_channel(channel_id);
  
  if (channel == NULL) {
    goto error;
  }

  /*Register the channel for all underlying agents (use FLEXRAN_AGENT_MAX)*/
  flexran_agent_register_channel(mod_id, channel, FLEXRAN_AGENT_MAX);

  /*Example of registration for a specific agent(MAC):
   *flexran_agent_register_channel(mod_id, channel, FLEXRAN_AGENT_MAC);
   */

  /*Initialize the continuous MAC stats update mechanism*/
  flexran_agent_init_cont_mac_stats_update(mod_id);
  
  new_thread(receive_thread, &flexran_agent[mod_id]);

  /*Initialize and register the mac xface. Must be modified later
   *for more flexibility in agent management */

  AGENT_MAC_xface *mac_agent_xface = (AGENT_MAC_xface *) malloc(sizeof(AGENT_MAC_xface));
  flexran_agent_register_mac_xface(mod_id, mac_agent_xface);
  
  /* 
   * initilize a timer 
   */ 
  
  flexran_agent_init_timer();

  /*
   * Initialize the mac agent
   */
  flexran_agent_init_mac_agent(mod_id);
  
  /* 
   * start the enb agent task for tx and interaction with the underlying network function
   */ 
  if (!agent_task_created) {
    if (itti_create_task (TASK_FLEXRAN_AGENT, flexran_agent_task, (void *) &flexran_agent[mod_id]) < 0) {
      LOG_E(FLEXRAN_AGENT, "Create task for FlexRAN Agent failed\n");
      return -1;
    }
    agent_task_created = 1;
  }
  
  LOG_I(FLEXRAN_AGENT,"client ends\n");
  return 0;

error:
  LOG_I(FLEXRAN_AGENT,"there was an error\n");
  return 1;

}

Protocol__FlexranMessage *flexran_agent_timeout(void* args){

  //  flexran_agent_timer_args_t *timer_args = calloc(1, sizeof(*timer_args));
  //memcpy (timer_args, args, sizeof(*timer_args));
  flexran_agent_timer_args_t *timer_args = (flexran_agent_timer_args_t *) args;
  
  LOG_I(FLEXRAN_AGENT, "flexran_agent %d timeout\n", timer_args->mod_id);
  //LOG_I(FLEXRAN_AGENT, "eNB action %d ENB flags %d \n", timer_args->cc_actions,timer_args->cc_report_flags);
  //LOG_I(FLEXRAN_AGENT, "UE action %d UE flags %d \n", timer_args->ue_actions,timer_args->ue_report_flags);
  
  return NULL;
}