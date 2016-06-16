/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to Event Headers - since
 * Heartbeats only contain the Event Header, the Heartbeat factory function is
 * here too.
 *
 * License
 * -------
 *
 * Copyright(c) <2016>, AT&T Intellectual Property.  All other rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:  This product includes
 *    software developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#include "evel.h"
#include "evel_internal.h"
#include "metadata.h"

/**************************************************************************//**
 * Unique sequence number for events from this VNF.
 *****************************************************************************/
static int event_sequence = 0;


/**************************************************************************//**
 * Create a new heartbeat event.
 *
 * @note that the heartbeat is just a "naked" commonEventHeader!
 *
 * @returns pointer to the newly manufactured ::EVENT_HEADER.  If the event is
 *          not used it must be released using ::evel_free_event
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEADER * evel_new_heartbeat()
{
  EVENT_HEADER * heartbeat = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Allocate the header.                                                    */
  /***************************************************************************/
  heartbeat = malloc(sizeof(EVENT_HEADER));
  if (heartbeat == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(heartbeat, 0, sizeof(EVENT_HEADER));

  /***************************************************************************/
  /* Initialize the header.  Get a new event sequence number.  Note that if  */
  /* any memory allocation fails in here we will fail gracefully because     */
  /* everything downstream can cope with NULLs.                              */
  /***************************************************************************/
  evel_init_header(heartbeat);
  heartbeat->event_type = strdup("Autonomous heartbeat");

exit_label:
  EVEL_EXIT();
  return heartbeat;
}

/**************************************************************************//**
 * Initialize a newly created event header.
 *
 * @param header  Pointer to the header being initialized.
 *****************************************************************************/
void evel_init_header(EVENT_HEADER * header)
{
  char scratchpad[EVEL_MAX_STRING_LEN + 1] = {0};
  struct timeval tv;

  EVEL_ENTER();

  assert(header != NULL);

  gettimeofday(&tv, NULL);

  /***************************************************************************/
  /* Initialize the header.  Get a new event sequence number.  Note that if  */
  /* any memory allocation fails in here we will fail gracefully because     */
  /* everything downstream can cope with NULLs.                              */
  /***************************************************************************/
  event_sequence++;
  header->event_domain = EVEL_DOMAIN_HEARTBEAT;
  snprintf(scratchpad, EVEL_MAX_STRING_LEN, "%d", event_sequence);
  header->event_id = strdup(scratchpad);
  header->functional_role = strdup(functional_role);
  header->last_epoch_microsec = tv.tv_usec + 1000000 * tv.tv_sec;
  header->priority = EVEL_PRIORITY_NORMAL;
  header->reporting_entity_id = strdup(openstack_vm_uuid());
  header->reporting_entity_name = strdup(openstack_vm_name());
  header->sequence = event_sequence;
  header->source_id = strdup(header->reporting_entity_id);
  header->source_name = strdup(header->reporting_entity_name);
  header->start_epoch_microsec = header->last_epoch_microsec;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the header as a JSON commonEventHeader.
 *
 * @param json      Pointer to where to staore the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_header::json.
 * @param event     Pointer to the ::EVENT_HEADER to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_header(char * json,
                                   int max_size,
                                   EVENT_HEADER * event)
{
  int offset = 0;

  assert(json != NULL);
  assert(max_size > 0);
  assert(event != NULL);

  offset += snprintf(json + offset, max_size - offset,
                     "\"commonEventHeader\":{");

  switch (event->event_domain)
  {
  case EVEL_DOMAIN_HEARTBEAT:
    offset += snprintf(json + offset, max_size - offset,
                       "\"domain\": \"heartbeat\", ");
    break;

  case EVEL_DOMAIN_FAULT:
    offset += snprintf(json + offset, max_size - offset,
                       "\"domain\": \"fault\", ");
    break;

  case EVEL_DOMAIN_MEASUREMENT:
    offset += snprintf(json + offset, max_size - offset,
                       "\"domain\": \"measurementsForVfScaling\", ");
    break;

  case EVEL_DOMAIN_REPORT:
    offset += snprintf(json + offset, max_size - offset,
                       "\"domain\": \"measurementsForVfReporting\", ");
    break;

  default:
    EVEL_ERROR("Unexpected domain %d", event->event_domain);
    assert(0);
  }
  offset += snprintf(json + offset, max_size - offset,
                     "\"eventId\": \"%s\", ", event->event_id);
  if (event->event_type != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventType\": \"%s\", ", event->event_type);
  }
  offset += snprintf(json + offset, max_size - offset,
                     "\"functionalRole\": \"%s\", ", event->functional_role);
  offset += snprintf(json + offset, max_size - offset,
                     "\"lastEpochMicrosec\": %llu, ",
                     event->last_epoch_microsec);
  offset += snprintf(json + offset, max_size - offset,
                     "\"reportingEntityId\": \"%s\", ",
                     event->reporting_entity_id);
  offset += snprintf(json + offset, max_size - offset,
                     "\"reportingEntityName\": \"%s\", ",
                     event->reporting_entity_name);
  offset += snprintf(json + offset, max_size - offset,
                     "\"sequence\": %d, ", event->sequence);
  offset += snprintf(json + offset, max_size - offset,
                     "\"sourceId\": \"%s\", ", event->source_id);
  offset += snprintf(json + offset, max_size - offset,
                     "\"sourceName\": \"%s\", ", event->source_name);
  offset += snprintf(json + offset, max_size - offset,
                     "\"startEpochMicrosec\": %llu, ",
                     event->start_epoch_microsec);
  switch (event->priority)
  {
  case EVEL_PRIORITY_HIGH:
    offset += snprintf(json + offset, max_size - offset,
                       "\"priority\": \"High\", ");
    break;

  case EVEL_PRIORITY_MEDIUM:
    offset += snprintf(json + offset, max_size - offset,
                       "\"priority\": \"Medium\", ");
    break;

  case EVEL_PRIORITY_NORMAL:
    offset += snprintf(json + offset, max_size - offset,
                       "\"priority\": \"Normal\", ");
    break;

  case EVEL_PRIORITY_LOW:
    offset += snprintf(json + offset, max_size - offset,
                       "\"priority\": \"Low\", ");
    break;

  default:
    EVEL_ERROR("Unexpected priority %d", event->priority);
    assert(0);
  }
  offset += snprintf(json + offset, max_size - offset,
                     "\"version\": %d", EVEL_API_VERSION);

  offset += snprintf(json + offset, max_size - offset,
                     "}");
  return offset;
}


/**************************************************************************//**
 * Free an event header.
 *
 * Free off the event header supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the header itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_header(EVENT_HEADER * event)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);

  /***************************************************************************/
  /* Free all internal strings.                                              */
  /***************************************************************************/
  free(event->event_id);
  free(event->event_type);
  free(event->functional_role);
  free(event->reporting_entity_id);
  free(event->reporting_entity_name);
  free(event->source_id);
  free(event->source_name);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the event as a JSON event object according to AT&T's schema.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in json_body.
 * @param event     Pointer to the ::EVENT_HEADER to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_event(char * json,
                                  int max_size,
                                  EVENT_HEADER * event)
{
  int offset = 0;

  offset += snprintf(json + offset, max_size - offset,
                     "{\"event\":{");

  switch (event->event_domain)
  {
  case EVEL_DOMAIN_HEARTBEAT:
    offset += evel_json_encode_header(json + offset, max_size - offset, event);
    break;

  case EVEL_DOMAIN_FAULT:
    offset += evel_json_encode_fault(json + offset,
                                     max_size - offset,
                                     (EVENT_FAULT *)event);
    break;

  case EVEL_DOMAIN_MEASUREMENT:
    offset += evel_json_encode_measurement(json + offset,
                                           max_size - offset,
                                           (EVENT_MEASUREMENT *)event);
     break;

  case EVEL_DOMAIN_REPORT:
    offset += evel_json_encode_report(json + offset,
                                      max_size - offset,
                                      (EVENT_REPORT *)event);
     break;

  default:
    EVEL_ERROR("Unexpected domain %d", event->event_domain);
    assert(0);
  }

  offset += snprintf(json + offset, max_size - offset, "}}");

  return offset;
}

