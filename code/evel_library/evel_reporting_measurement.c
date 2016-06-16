/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the Measurement for VF
 * Reporting event.
 *
 * @note  This is an experimental event tytpe and does not form part of the
 *        currently approved AT&T event schema.  It is intended to allow a
 *        less-onerous event reporting mechanism because it avoids having to
 *        return all the platform statistics which are mandatory in the
 *        **measurementsForVfScaling** event.
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
 * Create a new Report event.
 *
 * @note    The mandatory fields on the Report must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Report has immutable properties.
 *
 * @param   measurement_interval

 * @returns pointer to the newly manufactured ::EVENT_REPORT.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_REPORT * evel_new_report(double measurement_interval)
{
  EVENT_REPORT * report = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement_interval >= 0.0);

  /***************************************************************************/
  /* Allocate the report.                                                     */
  /***************************************************************************/
  report = malloc(sizeof(EVENT_REPORT));
  if (report == NULL)
  {
    log_error_state("Out of memory for Report");
    goto exit_label;
  }
  memset(report, 0, sizeof(EVENT_REPORT));
  EVEL_DEBUG("New report is at %lp", report);

  /***************************************************************************/
  /* Initialize the header & the report fields.                         */
  /***************************************************************************/
  evel_init_header(&report->header);
  report->header.event_domain = EVEL_DOMAIN_REPORT;
  report->header.priority = EVEL_PRIORITY_NORMAL;
  report->measurement_interval = measurement_interval;

  dlist_initialize(&report->feature_usage);
  report->measurement_fields_version = EVEL_API_VERSION;

exit_label:
  EVEL_EXIT();
  return report;
}

/**************************************************************************//**
 * Set the Event Type property of the Report.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param report Pointer to the Report.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns
 *****************************************************************************/
void evel_report_type_set(EVENT_REPORT * report,
                               const char const * type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(report != NULL);
  assert(report->header.event_domain == EVEL_DOMAIN_REPORT);
  assert(type != NULL);

  if (report->header.event_type == NULL)
  {
    EVEL_DEBUG("Setting Event Type to %s", type);
    report->header.event_type = strdup(type);
  }
  else
  {
    EVEL_ERROR("Ignoring attempt to update Event Type to %s. "
               "Event Type already set to %s",
               type, report->header.event_type);
  }
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param report          Pointer to the report.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_report_feature_use_add(EVENT_REPORT * report,
                                 char * feature,
                                 double utilization)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(report != NULL);
  assert(report->header.event_domain == EVEL_DOMAIN_REPORT);
  assert(feature != NULL);
  assert(utilization >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Feature=%s Use=%lf", feature, utilization);
  feature_use = malloc(sizeof(MEASUREMENT_FEATURE_USE));
  assert(feature_use != NULL);
  feature_use->feature_id = strdup(feature);
  assert(feature_use->feature_id != NULL);
  feature_use->feature_utilization = utilization;

  dlist_push_first(&report->feature_usage, feature_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param report   Pointer to the report.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param value    ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_report_custom_measurement_add(EVENT_REPORT * report,
                                        const char const * group,
                                        const char const * name,
                                        const char const * value)
{
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * measurement = NULL;
  DLIST_ITEM * item = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(report != NULL);
  assert(report->header.event_domain == EVEL_DOMAIN_REPORT);
  assert(group != NULL);
  assert(name != NULL);
  assert(value != NULL);

  /***************************************************************************/
  /* Allocate a container for the name/value pair.                           */
  /***************************************************************************/
  EVEL_DEBUG("Adding Measurement Group=%s Name=%s Value=%s",
              group, name, value);
  measurement = malloc(sizeof(CUSTOM_MEASUREMENT));
  assert(measurement != NULL);
  measurement->name = strdup(name);
  assert(measurement->name != NULL);
  measurement->value = strdup(value);
  assert(measurement->value != NULL);

  /***************************************************************************/
  /* See if we have that group already.                                      */
  /***************************************************************************/
  item = dlist_get_first(&report->measurement_groups);
  while (item != NULL)
  {
    measurement_group = (MEASUREMENT_GROUP *) item->item;
    EVEL_DEBUG("Got measurement group %s", measurement_group->name);
    if (strcmp(group, measurement_group->name) == 0)
    {
      EVEL_DEBUG("Found existing Measurement Group");
      break;
    }
    item = dlist_get_next(item);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  if (item == NULL)
  {
    EVEL_DEBUG("Creating new Measurement Group");
    measurement_group = malloc(sizeof(MEASUREMENT_GROUP));
    assert(measurement_group != NULL);
    measurement_group->name = strdup(group);
    assert(measurement_group->name != NULL);
    dlist_initialize(&measurement_group->measurements);
    dlist_push_first(&report->measurement_groups, measurement_group);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  dlist_push_first(&measurement_group->measurements, measurement);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the report as a JSON report.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_report::json.
 * @param event     Pointer to the ::EVENT_REPORT to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_report(char * json, int max_size, EVENT_REPORT * event)
{
  int offset = 0;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  DLIST_ITEM * nested_item = NULL;

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json != NULL);
  assert(max_size > 0);
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_REPORT);

  offset += evel_json_encode_header(json + offset,
                                    max_size - offset,
                                    &event->header);

  offset += snprintf(json + offset, max_size - offset,
                     ", \"measurementsForVfReporting\":{");

  offset += snprintf(json + offset, max_size - offset,
                     "\"measurementInterval\": %lf, ",
                     event->measurement_interval);

  /***************************************************************************/
  /* Feature Utilization list.                                               */
  /***************************************************************************/
  item = dlist_get_first(&event->feature_usage);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"featureUsageArray\": [");
    while (item != NULL)
    {
      feature_use = (MEASUREMENT_FEATURE_USE*) item->item;
      offset += snprintf(json + offset, max_size - offset,
                          "{\"featureIdentifier\": \"%s\", ",
                          feature_use->feature_id);
      offset += snprintf(json + offset, max_size - offset,
                          "\"featureUtilization\": %lf}",
                          feature_use->feature_utilization);
      item = dlist_get_next(item);
      if (item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  /***************************************************************************/
  /* Additional Measurement Groups list.                                     */
  /***************************************************************************/
  item = dlist_get_first(&event->measurement_groups);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"additionalMeasurements\": [");
    while (item != NULL)
    {
      measurement_group = (MEASUREMENT_GROUP *) item->item;
      offset += snprintf(json + offset, max_size - offset,
                          "{\"name\": \"%s\", ",
                          measurement_group->name);
      offset += snprintf(json + offset, max_size - offset,
                         "\"measurements\": [");

      /***********************************************************************/
      /* Measurements list.                                                  */
      /***********************************************************************/
      nested_item = dlist_get_first(&measurement_group->measurements);
      while (nested_item != NULL)
      {
        custom_measurement = (CUSTOM_MEASUREMENT *) nested_item->item;
        offset += snprintf(json + offset, max_size - offset,
                           "{\"name\": \"%s\", ",
                           custom_measurement->name);
        offset += snprintf(json + offset, max_size - offset,
                           "\"value\": \"%s\"}",
                           custom_measurement->value);

        nested_item = dlist_get_next(nested_item);
        if (nested_item != NULL)
        {
          offset += snprintf(json + offset, max_size - offset, ", ");
        }
      }
      offset += snprintf(json + offset, max_size - offset, "]}");

      item = dlist_get_next(item);
      if (item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  /***************************************************************************/
  /* Although optional, we always generate the version.  Note that this      */
  /* closes the object, too.                                                 */
  /***************************************************************************/
  offset += snprintf(json + offset, max_size - offset,
                     "\"measurementFieldsVersion\": %d}",
                     EVEL_API_VERSION);
  return offset;
}

/**************************************************************************//**
 * Free a Report.
 *
 * Free off the Report supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the Report itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_report(EVENT_REPORT * event)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_REPORT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  feature_use = dlist_pop_last(&event->feature_usage);
  while (feature_use != NULL)
  {
    EVEL_DEBUG("Freeing Feature use Info (%s)", feature_use->feature_id);
    free(feature_use->feature_id);
    free(feature_use);
    feature_use = dlist_pop_last(&event->feature_usage);
  }
  measurement_group = dlist_pop_last(&event->measurement_groups);
  while (measurement_group != NULL)
  {
    EVEL_DEBUG("Freeing Measurement Group (%s)", measurement_group->name);

    custom_measurement = dlist_pop_last(&measurement_group->measurements);
    while (custom_measurement != NULL)
    {
      EVEL_DEBUG("Freeing mesaurement (%s)", custom_measurement->name);

      free(custom_measurement->name);
      free(custom_measurement->value);
      free(custom_measurement);
      custom_measurement = dlist_pop_last(&measurement_group->measurements);
    }

    free(measurement_group->name);
    free(measurement_group);
    measurement_group = dlist_pop_last(&event->measurement_groups);
  }

  evel_free_header(&event->header);

  EVEL_EXIT();
}

