/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the Measurement.
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
 * Create a new Measurement event.
 *
 * @note    The mandatory fields on the Measurement must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Measurement has immutable properties.
 *
 * @param   concurrent_sessions
 * @param   configured_entities
 * @param   mean_request_latency
 * @param   measurement_interval
 * @param   memory_configured
 * @param   memory_used
 * @param   request_rate

 * @returns pointer to the newly manufactured ::EVENT_MEASUREMENT.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MEASUREMENT * evel_new_measurement(int concurrent_sessions,
                                         int configured_entities,
                                         double mean_request_latency,
                                         double measurement_interval,
                                         double memory_configured,
                                         double memory_used,
                                         int request_rate)
{
  EVENT_MEASUREMENT * measurement = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(concurrent_sessions >= 0);
  assert(configured_entities >= 0);
  assert(mean_request_latency >= 0.0);
  assert(measurement_interval >= 0.0);
  assert(memory_configured >= 0.0);
  assert(memory_used >= 0.0);
  assert(request_rate >= 0);

  /***************************************************************************/
  /* Allocate the measurement.                                                     */
  /***************************************************************************/
  measurement = malloc(sizeof(EVENT_MEASUREMENT));
  if (measurement == NULL)
  {
    log_error_state("Out of memory for Measurement");
    goto exit_label;
  }
  memset(measurement, 0, sizeof(EVENT_MEASUREMENT));
  EVEL_DEBUG("New measurement is at %lp", measurement);

  /***************************************************************************/
  /* Initialize the header & the measurement fields.                         */
  /***************************************************************************/
  evel_init_header(&measurement->header);
  measurement->header.event_domain = EVEL_DOMAIN_MEASUREMENT;
  measurement->header.priority = EVEL_PRIORITY_NORMAL;
  measurement->concurrent_sessions = concurrent_sessions;
  measurement->configured_entities = configured_entities;
  dlist_initialize(&measurement->cpu_usage);
  dlist_initialize(&measurement->filesystem_usage);
  dlist_initialize(&measurement->latency_distribution);
  measurement->mean_request_latency = mean_request_latency;
  measurement->measurement_interval = measurement_interval;
  measurement->memory_configured = memory_configured;
  measurement->memory_used = memory_used;
  measurement->request_rate = request_rate;
  dlist_initialize(&measurement->vnic_usage);

  measurement->aggregate_cpu_usage = 0.0;
  dlist_initialize(&measurement->codec_usage);
  dlist_initialize(&measurement->feature_usage);
  dlist_initialize(&measurement->measurement_groups);
  measurement->measurement_fields_version = EVEL_API_VERSION;
  measurement->media_ports_in_use = 0;
  measurement->vnfc_scaling_metric = 0.0;

exit_label:
  EVEL_EXIT();
  return measurement;
}

/**************************************************************************//**
 * Set the Event Type property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement Pointer to the Measurement.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns
 *****************************************************************************/
void evel_measurement_type_set(EVENT_MEASUREMENT * measurement,
                               const char const * type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(type != NULL);

  if (measurement->header.event_type == NULL)
  {
    EVEL_DEBUG("Setting Event Type to %s", type);
    measurement->header.event_type = strdup(type);
  }
  else
  {
    EVEL_ERROR("Ignoring attempt to update Event Type to %s. "
               "Event Type already set to %s",
               type, measurement->header.event_type);
  }
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional CPU usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param name          ASCIIZ string with the CPU's name.
 * @param value         CPU utilization.
 *****************************************************************************/
void evel_measurement_cpu_use_add(EVENT_MEASUREMENT * measurement,
                                 char * name, double value)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(name != NULL);
  assert(value >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding name=%s value=%lf", name, value);
  cpu_use = malloc(sizeof(MEASUREMENT_CPU_USE));
  assert(cpu_use != NULL);
  cpu_use->name = strdup(name);
  cpu_use->value = value;
  assert(cpu_use->name != NULL);

  dlist_push_first(&measurement->cpu_usage, cpu_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional File System usage value name/value pair to the
 * Measurement.
 *
 * The ID is null delimited ASCII string.  The library takes a copy so the
 * caller does not have to preserve values after the function returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param vm_id           ASCIIZ string with the file-system's UUID.
 * @param block_configured  Block storage configured.
 * @param block_used        Block storage in use.
 * @param block_iops        Block storage IOPS.
 * @param ephemeral_configured  Ephemeral storage configured.
 * @param ephemeral_used        Ephemeral storage in use.
 * @param ephemeral_iops        Ephemeral storage IOPS.
 *****************************************************************************/
void evel_measurement_fsys_use_add(EVENT_MEASUREMENT * measurement,
                                 char * vm_id,
                                 double block_configured,
                                 double block_used,
                                 int block_iops,
                                 double ephemeral_configured,
                                 double ephemeral_used,
                                 int ephemeral_iops)
{
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(vm_id != NULL);
  assert(block_configured >= 0.0);
  assert(block_used >= 0.0);
  assert(block_iops >= 0);
  assert(ephemeral_configured >= 0.0);
  assert(ephemeral_used >= 0.0);
  assert(ephemeral_iops >= 0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding VM ID=%s", vm_id);
  fsys_use = malloc(sizeof(MEASUREMENT_FSYS_USE));
  assert(fsys_use != NULL);
  fsys_use->vm_id = strdup(vm_id);
  fsys_use->block_configured = block_configured;
  fsys_use->block_used = block_used;
  fsys_use->block_iops = block_iops;
  fsys_use->ephemeral_configured = block_configured;
  fsys_use->ephemeral_used = ephemeral_used;
  fsys_use->ephemeral_iops = ephemeral_iops;

  dlist_push_first(&measurement->filesystem_usage, fsys_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional Latency Distribution bucket to the Measurement.
 *
 * @param measurement   Pointer to the measurement.
 * @param low_end       Low end of the bucket's range.
 * @param high_end      High end of the bucket's range.
 * @param count         Count of events in this bucket.
 *****************************************************************************/
void evel_measurement_latency_add(EVENT_MEASUREMENT * measurement,
                                 double low_end,
                                 double high_end,
                                 int count)
{
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(low_end >= 0.0);
  assert(high_end >= 0.0);
  assert(count >= 0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Bucket %lf-%lf", low_end, high_end);
  bucket = malloc(sizeof(MEASUREMENT_LATENCY_BUCKET));
  assert(bucket != NULL);
  bucket->low_end = low_end;
  bucket->high_end = high_end;
  bucket->count = count;

  dlist_push_first(&measurement->latency_distribution, bucket);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional vNIC usage record Measurement.
 *
 * The ID is null delimited ASCII string.  The library takes a copy so the
 * caller does not have to preserve values after the function returns.
 *
 * @param measurement           Pointer to the measurement.
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param broadcast_packets_in  Broadcast packets received.
 * @param broadcast_packets_out Broadcast packets transmitted.
 * @param bytes_in              Total bytes received.
 * @param bytes_out             Total bytes transmitted.
 * @param multicast_packets_in  Multicast packets received.
 * @param multicast_packets_out Multicast packets transmitted.
 * @param unicast_packets_in    Unicast packets received.
 * @param unicast_packets_out   Unicast packets transmitted.
 *****************************************************************************/
void evel_measurement_vnic_use_add(EVENT_MEASUREMENT * measurement,
                                   char * vnic_id,
                                   int broadcast_packets_in,
                                   int broadcast_packets_out,
                                   int bytes_in,
                                   int bytes_out,
                                   int multicast_packets_in,
                                   int multicast_packets_out,
                                   int unicast_packets_in,
                                   int unicast_packets_out)
{
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(vnic_id != NULL);
  assert(broadcast_packets_in >= 0);
  assert(broadcast_packets_out >= 0);
  assert(bytes_in >= 0);
  assert(bytes_out >= 0);
  assert(multicast_packets_in >= 0);
  assert(multicast_packets_out >= 0);
  assert(unicast_packets_in >= 0);
  assert(unicast_packets_out >= 0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding VNIC ID=%s", vnic_id);
  vnic_use = malloc(sizeof(MEASUREMENT_VNIC_USE));
  assert(vnic_use != NULL);
  vnic_use->vnic_id = strdup(vnic_id);
  vnic_use->broadcast_packets_in = broadcast_packets_in;
  vnic_use->broadcast_packets_out = broadcast_packets_out;
  vnic_use->bytes_in = bytes_in;
  vnic_use->bytes_out = bytes_out;
  vnic_use->multicast_packets_in = multicast_packets_in;
  vnic_use->multicast_packets_out = multicast_packets_out;
  vnic_use->unicast_packets_in = unicast_packets_in;
  vnic_use->unicast_packets_out = unicast_packets_out;

  dlist_push_first(&measurement->vnic_usage, vnic_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_measurement_feature_use_add(EVENT_MEASUREMENT * measurement,
                                      char * feature,
                                      double utilization)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
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

  dlist_push_first(&measurement->feature_usage, feature_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the Measaurement.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param value    ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_measurement_custom_measurement_add(EVENT_MEASUREMENT * measurement,
                                             const char const * group,
                                             const char const * name,
                                             const char const * value)
{
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(group != NULL);
  assert(name != NULL);
  assert(value != NULL);

  /***************************************************************************/
  /* Allocate a container for the name/value pair.                           */
  /***************************************************************************/
  EVEL_DEBUG("Adding Measurement Group=%s Name=%s Value=%s",
              group, name, value);
  custom_measurement = malloc(sizeof(CUSTOM_MEASUREMENT));
  assert(custom_measurement != NULL);
  custom_measurement->name = strdup(name);
  assert(custom_measurement->name != NULL);
  custom_measurement->value = strdup(value);
  assert(custom_measurement->value != NULL);

  /***************************************************************************/
  /* See if we have that group already.                                      */
  /***************************************************************************/
  item = dlist_get_first(&measurement->measurement_groups);
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
    dlist_push_first(&measurement->measurement_groups, measurement_group);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  dlist_push_first(&measurement_group->measurements, custom_measurement);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Codec usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param codec           ASCIIZ string with the codec's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_measurement_codec_use_add(EVENT_MEASUREMENT * measurement,
                                    char * codec,
                                    int utilization)
{
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(codec != NULL);
  assert(utilization >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Codec=%s Use=%d", codec, utilization);
  codec_use = malloc(sizeof(MEASUREMENT_CODEC_USE));
  assert(codec_use != NULL);
  codec_use->codec_id = strdup(codec);
  assert(codec_use->codec_id != NULL);
  codec_use->codec_utilization = utilization;

  dlist_push_first(&measurement->codec_usage, codec_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Aggregate CPU Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement   Pointer to the measurement.
 * @param cpu_use       The CPU use to set.
 *****************************************************************************/
void evel_measurement_agg_cpu_use_set(EVENT_MEASUREMENT * measurement,
                                      double cpu_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(cpu_use >= 0.0);

  if (measurement->aggregate_cpu_usage > 0.0)
  {
    EVEL_ERROR("Ignoring attempt to update CPU Use to %lf. "
               "CPU Use already set to %lf",
               cpu_use, measurement->aggregate_cpu_usage);
  }
  else
  {
    EVEL_DEBUG("Setting Aggregate CPU Usage to %lf", cpu_use);
    measurement->aggregate_cpu_usage = cpu_use;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Media Ports in Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the measurement.
 * @param media_ports_in_use  The media port usage to set.
 *****************************************************************************/
void evel_measurement_media_port_use_set(EVENT_MEASUREMENT * measurement,
                                         int media_ports_in_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(media_ports_in_use >= 0);

  if (measurement->media_ports_in_use > 0)
  {
    EVEL_ERROR("Ignoring attempt to update Media Port use to %. "
               "Media Port use already set to %d",
               media_ports_in_use, measurement->media_ports_in_use);
  }
  else
  {
    EVEL_DEBUG("Setting Media Port Usage to %d", media_ports_in_use);
    measurement->media_ports_in_use = media_ports_in_use;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the VNFC Scaling Metric property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement     Pointer to the measurement.
 * @param scaling_metric  The scaling metric to set.
 *****************************************************************************/
void evel_measurement_vnfc_scaling_metric_set(EVENT_MEASUREMENT * measurement,
                                              double scaling_metric)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(scaling_metric >= 0.0);

  if (measurement->vnfc_scaling_metric > 0.0)
  {
    EVEL_ERROR("Ignoring attempt to update VNFC Scaling Metric to %lf. "
               "VNFC Scaling Metric  already set to %lf",
               scaling_metric, measurement->vnfc_scaling_metric);
  }
  else
  {
    EVEL_DEBUG("Setting VNFC Scaling Metric  to %lf", scaling_metric);
    measurement->vnfc_scaling_metric = scaling_metric;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the measurement as a JSON measurement.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_measurement::json.
 * @param event     Pointer to the ::EVENT_MEASUREMENT to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_measurement(char * json,
                                 int max_size,
                                 EVENT_MEASUREMENT * event)
{
  int offset = 0;
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_CODEC_USE * codec_use = NULL;
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
  assert(event->header.event_domain == EVEL_DOMAIN_MEASUREMENT);

  offset += evel_json_encode_header(json + offset,
                                    max_size - offset,
                                    &event->header);

  offset += snprintf(json + offset, max_size - offset,
                     ", \"measurementsForVfScaling\":{");

  offset += snprintf(json + offset, max_size - offset,
                     "\"concurrentSessions\": %d, ",
                     event->concurrent_sessions);
  offset += snprintf(json + offset, max_size - offset,
                     "\"configuredEntities\": %d, ",
                     event->configured_entities);

  /***************************************************************************/
  /* CPU Use list.                                                           */
  /***************************************************************************/
  item = dlist_get_first(&event->cpu_usage);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"cpuUsageArray\": [");
    while (item != NULL)
    {
      cpu_use = (MEASUREMENT_CPU_USE*) item->item;
      offset += snprintf(json + offset, max_size - offset,
                          "{\"name\": \"%s\", ", cpu_use->name);
      offset += snprintf(json + offset, max_size - offset,
                          "\"value\": %lf}", cpu_use->value);
      item = dlist_get_next(item);
      if (item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  /***************************************************************************/
  /* Filesystem Usage list.                                                  */
  /***************************************************************************/
  item = dlist_get_first(&event->filesystem_usage);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"filesystemUsageArray\": [");
    while (item != NULL)
    {
      fsys_use = (MEASUREMENT_FSYS_USE *) item->item;
      offset += snprintf(json + offset, max_size - offset,
                         "{\"blockConfigured\": %lf, ",
                         fsys_use->block_configured);
      offset += snprintf(json + offset, max_size - offset,
                         "\"blockIops\": %d, ", fsys_use->block_iops);
      offset += snprintf(json + offset, max_size - offset,
                         "\"blockUsed\": %lf, ", fsys_use->block_used);
      offset += snprintf(json + offset, max_size - offset,
                         "\"ephemeralConfigured\": %lf, ",
                         fsys_use->ephemeral_configured);
      offset += snprintf(json + offset, max_size - offset,
                         "\"ephemeralIops\": %d, ", fsys_use->ephemeral_iops);
      offset += snprintf(json + offset, max_size - offset,
                         "\"ephemeralUsed\": %lf, ", fsys_use->ephemeral_used);
      offset += snprintf(json + offset, max_size - offset,
                          "\"vmIdentifier\": \"%s\"}", fsys_use->vm_id);
      item = dlist_get_next(item);
      if (item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  /***************************************************************************/
  /* Latency distribution.                                                   */
  /***************************************************************************/
  item = dlist_get_first(&event->latency_distribution);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"latencyBucketMeasure\": [");
    while (item != NULL)
    {
      bucket = (MEASUREMENT_LATENCY_BUCKET*) item->item;
      offset += snprintf(json + offset, max_size - offset,
                         "{\"lowEndOfLatencyBucket\": %lf, ", bucket->low_end);
      offset += snprintf(json + offset, max_size - offset,
                         "\"highEndOfLatencyBucket\": %lf, ", bucket->high_end);
      offset += snprintf(json + offset, max_size - offset,
                         "\"countsInTheBucket\": %d}", bucket->count);
      item = dlist_get_next(item);
      if (item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  offset += snprintf(json + offset, max_size - offset,
                     "\"meanRequestLatency\": %lf, ",
                     event->mean_request_latency);
  offset += snprintf(json + offset, max_size - offset,
                     "\"measurementInterval\": %lf, ",
                     event->measurement_interval);
  offset += snprintf(json + offset, max_size - offset,
                     "\"memoryConfigured\": %lf, ",
                     event->memory_configured);
  offset += snprintf(json + offset, max_size - offset,
                     "\"memoryUsed\": %lf, ",
                     event->memory_used);
  offset += snprintf(json + offset, max_size - offset,
                     "\"requestRate\": %d, ",
                     event->request_rate);

  /***************************************************************************/
  /* vNIC Usage                                                              */
  /***************************************************************************/
  item = dlist_get_first(&event->vnic_usage);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"vNicUsageArray\": [");
    while (item != NULL)
    {
      vnic_use = (MEASUREMENT_VNIC_USE *) item->item;
      offset += snprintf(json + offset, max_size - offset,
                         "{\"broadcastPacketsIn\": %d, ",
                         vnic_use->broadcast_packets_in);
      offset += snprintf(json + offset, max_size - offset,
                         "\"broadcastPacketsOut\": %d, ",
                         vnic_use->broadcast_packets_out);
      offset += snprintf(json + offset, max_size - offset,
                         "\"bytesIn\": %d, ",
                         vnic_use->bytes_in);
      offset += snprintf(json + offset, max_size - offset,
                         "\"bytesOut\": %d, ", vnic_use->bytes_out);
      offset += snprintf(json + offset, max_size - offset,
                         "\"multicastPacketsIn\": %d, ",
                         vnic_use->multicast_packets_in);
      offset += snprintf(json + offset, max_size - offset,
                         "\"multicastPacketsOut\": %d, ",
                         vnic_use->multicast_packets_out);
      offset += snprintf(json + offset, max_size - offset,
                         "\"unicastPacketsIn\": %d, ",
                         vnic_use->unicast_packets_in);
      offset += snprintf(json + offset, max_size - offset,
                         "\"unicastPacketsOut\": %d, ",
                         vnic_use->unicast_packets_out);
      offset += snprintf(json + offset, max_size - offset,
                         "\"vNicIdentifier\": \"%s\"}", vnic_use->vnic_id);
      item = dlist_get_next(item);
      if (item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }
    offset += snprintf(json + offset, max_size - offset, "], ");
  }

  if (event->aggregate_cpu_usage > 0.0)
  {
    offset += snprintf(json + offset, max_size - offset,
                     "\"aggregateCpuUsage\": %lf, ",
                     event->aggregate_cpu_usage);
  }
  if (event->media_ports_in_use > 0)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"numberOfMediaPortsInUse\": %d, ",
                       event->media_ports_in_use);
  }
  if (event->vnfc_scaling_metric > 0.0)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"vnfcScalingMetric\": %lf, ",
                       event->vnfc_scaling_metric);
  }

  /***************************************************************************/
  /* Feature Utilization list.                                                           */
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
  /* Codec Utilization list.                                                 */
  /***************************************************************************/
  item = dlist_get_first(&event->codec_usage);
  if (item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       "\"codecsInUse\": [");
    while (item != NULL)
    {
      codec_use = (MEASUREMENT_CODEC_USE*) item->item;
      offset += snprintf(json + offset, max_size - offset,
                          "{\"codecIdentifier\": \"%s\", ",
                          codec_use->codec_id);
      offset += snprintf(json + offset, max_size - offset,
                          "\"codecUtilization\": %d}",
                          codec_use->codec_utilization);
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
 * Free a Measurement.
 *
 * Free off the Measurement supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the Measurement itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement(EVENT_MEASUREMENT * event)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_CODEC_USE * codec_use = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MEASUREMENT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  cpu_use = dlist_pop_last(&event->cpu_usage);
  while (cpu_use != NULL)
  {
    EVEL_DEBUG("Freeing CPU use Info (%s)", cpu_use->name);
    free(cpu_use->name);
    free(cpu_use);
    cpu_use = dlist_pop_last(&event->cpu_usage);
  }

  fsys_use = dlist_pop_last(&event->filesystem_usage);
  while (fsys_use != NULL)
  {
    EVEL_DEBUG("Freeing Filesystem Use info (%s)", fsys_use->vm_id);
    free(fsys_use->vm_id);
    free(fsys_use);
    fsys_use = dlist_pop_last(&event->filesystem_usage);
  }

  bucket = dlist_pop_last(&event->latency_distribution);
  while (bucket != NULL)
  {
    EVEL_DEBUG("Freeing Latency Bucket");
    free(bucket);
    bucket = dlist_pop_last(&event->latency_distribution);
  }

  vnic_use = dlist_pop_last(&event->vnic_usage);
  while (vnic_use != NULL)
  {
    EVEL_DEBUG("Freeing vNIC use Info (%s)", vnic_use->vnic_id);
    free(vnic_use->vnic_id);
    free(vnic_use);
    vnic_use = dlist_pop_last(&event->vnic_usage);
  }

  codec_use = dlist_pop_last(&event->codec_usage);
  while (codec_use != NULL)
  {
    EVEL_DEBUG("Freeing Codec use Info (%s)", codec_use->codec_id);
    free(codec_use->codec_id);
    free(codec_use);
    codec_use = dlist_pop_last(&event->codec_usage);
  }

  feature_use = dlist_pop_last(&event->feature_usage);
  while (feature_use != NULL)
  {
    EVEL_DEBUG("Freeing Feature use Info (%s)", feature_use->feature_id);
    free(feature_use->feature_id);
    free(feature_use);
    feature_use = dlist_pop_last(&event->feature_usage);
  }

  evel_free_header(&event->header);

  EVEL_EXIT();
}

