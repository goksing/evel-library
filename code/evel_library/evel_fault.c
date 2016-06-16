/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the Fault.
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

#include "evel.h"
#include "evel_internal.h"


/**************************************************************************//**
 * Create a new fault event.
 *
 * @note    The mandatory fields on the Fault must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Fault has immutable properties.
 * @param   condition   The condition indicated by the Fault.
 * @param   specific_problem  The specific problem triggering the fault.
 * @param   priority    The priority of the event.
 * @param   severity    The severity of the Fault.
 * @returns pointer to the newly manufactured ::EVENT_FAULT.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_FAULT * evel_new_fault(const char const * condition,
                             const char const * specific_problem,
                             EVEL_EVENT_PRIORITIES priority,
                             EVEL_FAULT_SEVERITIES severity)
{
  EVENT_FAULT * fault = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(condition != NULL);
  assert(specific_problem != NULL);
  assert(priority < EVEL_MAX_PRIORITIES);
  assert(severity < EVEL_MAX_SEVERITIES);

  /***************************************************************************/
  /* Allocate the fault.                                                     */
  /***************************************************************************/
  fault = malloc(sizeof(EVENT_FAULT));
  if (fault == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(fault, 0, sizeof(EVENT_FAULT));
  EVEL_DEBUG("New fault is at %lp", fault);

  /***************************************************************************/
  /* Initialize the header & the fault fields.  Optional string values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  evel_init_header(&fault->header);
  fault->header.event_domain = EVEL_DOMAIN_FAULT;
  fault->header.priority = priority;
  dlist_initialize(&fault->additional_info);
  fault->event_severity = severity;
  fault->event_source_type = event_source_type;
  fault->vf_status = EVEL_VF_STATUS_ACTIVE;
  fault->alarm_condition = strdup(condition);
  fault->specific_problem = strdup(specific_problem);

exit_label:
  EVEL_EXIT();
  return fault;
}

/**************************************************************************//**
 * Add an additional value name/value pair to the Fault.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param fault     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_fault_addl_info_add(EVENT_FAULT * fault, char * name, char * value)
{
  FAULT_ADDL_INFO * addl_info = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  addl_info = malloc(sizeof(FAULT_ADDL_INFO));
  assert(addl_info != NULL);
  addl_info->name = strdup(name);
  addl_info->value = strdup(value);
  assert(addl_info->name != NULL);
  assert(addl_info->value != NULL);

  dlist_push_first(&fault->additional_info, addl_info);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Alarm Interface A property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param interface  The Alarm Interface A to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_interface_set(EVENT_FAULT * fault,
                              const char const * interface)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  assert(interface != NULL);

  if (fault->alarm_interface_a == NULL)
  {
    EVEL_DEBUG("Setting Alarm Interface A to %s", interface);
    fault->alarm_interface_a = strdup(interface);
  }
  else
  {
    EVEL_ERROR("Ignoring attempt to update Alarm Interface A to %s. "
               "Alarm Interface A already set to %s",
               interface, fault->alarm_interface_a);
  }
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Event Type property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param type       The Event Type to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns
 *****************************************************************************/
void evel_fault_type_set(EVENT_FAULT * fault, const char const * type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  assert(type != NULL);

  if (fault->header.event_type == NULL)
  {
    EVEL_DEBUG("Setting Event Type to %s", type);
    fault->header.event_type = strdup(type);
  }
  else
  {
    EVEL_ERROR("Ignoring attempt to update Event Type to %s. "
               "Event Type already set to %s",
               type, fault->header.event_type);
  }
  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the fault in JSON according to AT&T's schema for the fault type.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_fault::json.
 * @param event     Pointer to the ::EVENT_FAULT to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_fault(char * json, int max_size, EVENT_FAULT * event)
{
  int offset = 0;
  FAULT_ADDL_INFO * addl_info = NULL;
  DLIST_ITEM * addl_info_item = NULL;

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json != NULL);
  assert(max_size > 0);
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_FAULT);

  offset += evel_json_encode_header(json + offset,
                                    max_size - offset,
                                    &event->header);

  offset += snprintf(json + offset, max_size - offset,
                     ", \"faultFields\":{");


  addl_info_item = dlist_get_first(&event->additional_info);
  if (addl_info_item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"alarmAdditionalInformation\": [");
    while (addl_info_item != NULL)
    {
      addl_info = (FAULT_ADDL_INFO*) addl_info_item->item;
      offset += snprintf(json + offset, max_size - offset,
                          "{\"name\": \"%s\", ", addl_info->name);
      offset += snprintf(json + offset, max_size - offset,
                          "\"value\": \"%s\"}", addl_info->value);
      addl_info_item = dlist_get_next(addl_info_item);
      if (addl_info_item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  offset += snprintf(json + offset, max_size - offset,
                     "\"alarmCondition\": \"%s\", ", event->alarm_condition);
  if (event->alarm_interface_a != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"alarmInterfaceA\": \"%s\", ",
                       event->alarm_interface_a);
  }

  switch (event->event_severity)
  {
  case EVEL_SEVERITY_CRITICAL:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSeverity\": \"CRITICAL\", ");
    break;

  case EVEL_SEVERITY_MAJOR:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSeverity\": \"MAJOR\", ");
    break;

  case EVEL_SEVERITY_MINOR:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSeverity\": \"MINOR\", ");
    break;

  case EVEL_SEVERITY_WARNING:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSeverity\": \"WARNING\", ");
    break;

  case EVEL_SEVERITY_NORMAL:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSeverity\": \"NORMAL\", ");
    break;

  default:
    EVEL_ERROR("Unexpected event severity %d", event->event_severity);
    assert(0);
  }

  switch (event->event_source_type)
  {
  case EVEL_SOURCE_OTHER:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"other(0)\", ");
    break;

  case EVEL_SOURCE_ROUTER:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"router(1)\", ");
    break;

  case EVEL_SOURCE_SWITCH:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"switch(2)\", ");
    break;

  case EVEL_SOURCE_HOST:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"host(3)\", ");
    break;

  case EVEL_SOURCE_CARD:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"card(4)\", ");
    break;

  case EVEL_SOURCE_PORT:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"port(5)\", ");
    break;

  case EVEL_SOURCE_SLOT_THRESHOLD:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"slotThreshold(6)\", ");
    break;

  case EVEL_SOURCE_PORT_THRESHOLD:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"portThreshold(7)\", ");
    break;

  case EVEL_SOURCE_VIRTUAL_MACHINE:
    offset += snprintf(json + offset, max_size - offset,
                       "\"eventSourceType\": \"virtualMachine(8)\", ");
    break;

  default:
    EVEL_ERROR("Unexpected event source type %d", event->event_source_type);
    assert(0);
  }

  offset += snprintf(json + offset, max_size - offset,
                     "\"faultFieldsVersion\": %d, ", EVEL_API_VERSION);
  offset += snprintf(json + offset, max_size - offset,
                     "\"specificProblem\": \"%s\", ",
                     event->specific_problem);

  switch (event->vf_status)
  {
  case EVEL_VF_STATUS_ACTIVE:
    offset += snprintf(json + offset, max_size - offset,
                       "\"vfStatus\": \"Active\"");
    break;

  case EVEL_VF_STATUS_IDLE:
    offset += snprintf(json + offset, max_size - offset,
                       "\"vfStatus\": \"Idle\"");
    break;

  case EVEL_VF_STATUS_PREP_TERMINATE:
    offset += snprintf(json + offset, max_size - offset,
                       "\"vfStatus\": \"Preparing to terminate\"");
    break;

  case EVEL_VF_STATUS_READY_TERMINATE:
    offset += snprintf(json + offset, max_size - offset,
                       "\"vfStatus\": \"Ready to terminate\"");
    break;

  case EVEL_VF_STATUS_REQ_TERMINATE:
    offset += snprintf(json + offset, max_size - offset,
                       "\"vfStatus\": \"Requesting termination\"");
    break;

  default:
    EVEL_ERROR("Unexpected VF Status %d", event->vf_status);
    assert(0);
  }

  offset += snprintf(json + offset, max_size - offset, "}");
  return offset;
}

/**************************************************************************//**
 * Free a Fault.
 *
 * Free off the Fault supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the Fault itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_fault(EVENT_FAULT * event)
{
  FAULT_ADDL_INFO * addl_info = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_FAULT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  addl_info = dlist_pop_last(&event->additional_info);
  while (addl_info != NULL)
  {
    EVEL_DEBUG("Freeing Additional Info (%s, %s)",
               addl_info->name,
               addl_info->value);
    free(addl_info->name);
    free(addl_info->value);
    free(addl_info);
    addl_info = dlist_pop_last(&event->additional_info);
  }
  free(event->alarm_condition);
  free(event->alarm_interface_a);
  free(event->specific_problem);
  evel_free_header(&event->header);

  EVEL_EXIT();
}

