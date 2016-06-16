#ifndef EVEL_INCLUDED
#define EVEL_INCLUDED
/**************************************************************************//**
 * @file
 * Header for EVEL library
 *
 * This file implements the EVEL library which is intended to provide a
 * simple wrapper around the complexity of AT&T's Vendor Event Listener API so
 * that VNFs can use it without worrying about details of the API transport.
 *
 * Zero return value is success (::EVEL_SUCCESS), non-zero is failure and will
 * be one of ::EVEL_ERR_CODES.
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

#include <stdio.h>
#include <stdarg.h>

#include "double_list.h"

/*****************************************************************************/
/* Supported API version.                                                    */
/*****************************************************************************/
#define EVEL_API_VERSION      1

/**************************************************************************//**
 * Error codes
 *
 * Error codes for EVEL low level interface
 *****************************************************************************/
typedef enum {
  EVEL_SUCCESS,                   /** The operation was successful.          */
  EVEL_ERR_GEN_FAIL,              /** Non-specific failure.                  */
  EVEL_CURL_LIBRARY_FAIL,         /** A cURL library operation failed.       */
  EVEL_PTHREAD_LIBRARY_FAIL,      /** A Posix threads operation failed.      */
  EVEL_OUT_OF_MEMORY,             /** A memory allocation failure occurred.  */
  EVEL_EVENT_BUFFER_FULL,         /** Too many events in the ring-buffer.    */
  EVEL_EVENT_HANDLER_INACTIVE,    /** Attempt to raise event when inactive.  */
  EVEL_NO_METADATA,               /** Failed to retrieve OpenStack metadata. */
  EVEL_BAD_METADATA,              /** OpenStack metadata invalid format.     */
  EVEL_BAD_JSON_FORMAT,           /** JSON failed to parse correctly.        */
  EVEL_JSON_KEY_NOT_FOUND,        /** Failed to find the specified JSON key. */
  EVEL_MAX_ERROR_CODES            /** Maximum number of valid error codes.   */
} EVEL_ERR_CODES;

/**************************************************************************//**
 * Logging levels
 *
 * Variable levels of verbosity in the logging functions.
 *****************************************************************************/
typedef enum {
  EVEL_LOG_MIN               = 0,
  EVEL_LOG_SPAMMY            = 30,
  EVEL_LOG_DEBUG             = 40,
  EVEL_LOG_INFO              = 50,
  EVEL_LOG_ERROR             = 60,
  EVEL_LOG_MAX               = 101
} EVEL_LOG_LEVELS;

/*****************************************************************************/
/* Maximum string lengths.                                                   */
/*****************************************************************************/
#define EVEL_MAX_STRING_LEN          4096
#define EVEL_MAX_JSON_BODY           16000
#define EVEL_MAX_ERROR_STRING_LEN    255
#define EVEL_MAX_URL_LEN             511

/**************************************************************************//**
 * How many events can be backed-up before we start dropping events on the
 * floor.
 *
 * @note  This value should be tuned in accordance with expected burstiness of
 *        the event load and the expected response time of the ECOMP event
 *        listener so that the probability of the buffer filling is suitably
 *        low.
 *****************************************************************************/
static const int EVEL_EVENT_BUFFER_DEPTH = 100;

/**************************************************************************//**
 * Event domains for the various events we support.
 * JSON equivalent field: domain
 *****************************************************************************/
typedef enum {
  EVEL_DOMAIN_INTERNAL,       /** Internal event, not for external routing.  */
  EVEL_DOMAIN_HEARTBEAT,      /** A Heartbeat event (event header only).     */
  EVEL_DOMAIN_FAULT,          /** A Fault event.                             */
  EVEL_DOMAIN_MEASUREMENT,    /** A Measurement for VF Scaling event.        */
  EVEL_DOMAIN_REPORT,         /** A Measurement for VF Reporting event.      */
  EVEL_MAX_DOMAINS            /** Maximum number of recognized Event types.  */
} EVEL_EVENT_DOMAINS;

/**************************************************************************//**
 * Event priorities.
 * JSON equivalent field: priority
 *****************************************************************************/
typedef enum {
  EVEL_PRIORITY_HIGH,
  EVEL_PRIORITY_MEDIUM,
  EVEL_PRIORITY_NORMAL,
  EVEL_PRIORITY_LOW,
  EVEL_MAX_PRIORITIES
} EVEL_EVENT_PRIORITIES;

/**************************************************************************//**
 * Fault severities.
 * JSON equivalent field: eventSeverity
 *****************************************************************************/
typedef enum {
  EVEL_SEVERITY_CRITICAL,
  EVEL_SEVERITY_MAJOR,
  EVEL_SEVERITY_MINOR,
  EVEL_SEVERITY_WARNING,
  EVEL_SEVERITY_NORMAL,
  EVEL_MAX_SEVERITIES
} EVEL_FAULT_SEVERITIES;

/**************************************************************************//**
 * Fault source types.
 * JSON equivalent field: eventSourceType
 *****************************************************************************/
typedef enum {
  EVEL_SOURCE_OTHER,
  EVEL_SOURCE_ROUTER,
  EVEL_SOURCE_SWITCH,
  EVEL_SOURCE_HOST,
  EVEL_SOURCE_CARD,
  EVEL_SOURCE_PORT,
  EVEL_SOURCE_SLOT_THRESHOLD,
  EVEL_SOURCE_PORT_THRESHOLD,
  EVEL_SOURCE_VIRTUAL_MACHINE,
  EVEL_MAX_SOURCE_TYPES
} EVEL_SOURCE_TYPES;

/**************************************************************************//**
 * Fault VNF Status.
 * JSON equivalent field: vfSTatus
 *****************************************************************************/
typedef enum {
  EVEL_VF_STATUS_ACTIVE,
  EVEL_VF_STATUS_IDLE,
  EVEL_VF_STATUS_PREP_TERMINATE,
  EVEL_VF_STATUS_READY_TERMINATE,
  EVEL_VF_STATUS_REQ_TERMINATE,
  EVEL_MAX_VF_STATUSES
} EVEL_VF_STATUSES;

/**************************************************************************//**
 * Event header.
 * JSON equivalent field: commonEventHeader
 *****************************************************************************/
typedef struct event_header {
  EVEL_EVENT_DOMAINS event_domain;
  char * event_id;
  char * event_type;
  char * functional_role;
  unsigned long long last_epoch_microsec;
  EVEL_EVENT_PRIORITIES priority;
  char * reporting_entity_id;
  char * reporting_entity_name;
  int sequence;
  char * source_id;
  char * source_name;
  unsigned long long start_epoch_microsec;
} EVENT_HEADER;

/**************************************************************************//**
 * Fault.
 * JSON equivalent field: faultFields
 *****************************************************************************/
typedef struct event_fault {
  EVENT_HEADER header;
  DLIST additional_info;
  char * alarm_condition;
  char * alarm_interface_a;
  EVEL_FAULT_SEVERITIES event_severity;
  EVEL_SOURCE_TYPES event_source_type;
  char * specific_problem;
  EVEL_VF_STATUSES vf_status;
} EVENT_FAULT;

/**************************************************************************//**
 * Fault Additional Info.
 * JSON equivalent field: alarmAdditionalInformation
 *****************************************************************************/
typedef struct fault_additional_info {
  char * name;
  char * value;
} FAULT_ADDL_INFO;

/**************************************************************************//**
 * Measurement.
 * JSON equivalent field: measurementsForVfScalingFields
 *****************************************************************************/
typedef struct event_measurement {
  EVENT_HEADER header;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  int concurrent_sessions;
  int configured_entities;
  DLIST cpu_usage;
  DLIST filesystem_usage;
  DLIST latency_distribution;
  double mean_request_latency;
  double measurement_interval;
  double memory_configured;
  double memory_used;
  int request_rate;
  DLIST vnic_usage;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  double aggregate_cpu_usage;
  DLIST codec_usage;
  DLIST feature_usage;
  DLIST measurement_groups;
  int measurement_fields_version;
  int media_ports_in_use;
  double vnfc_scaling_metric;

} EVENT_MEASUREMENT;

/**************************************************************************//**
 * CPU Usage.
 * JSON equivalent field: cpuUsage
 *****************************************************************************/
typedef struct measurement_cpu_use {
  char * name;
  double value;
} MEASUREMENT_CPU_USE;

/**************************************************************************//**
 * Filesystem Usage.
 * JSON equivalent field: filesystemUsage
 *****************************************************************************/
typedef struct measurement_fsys_use {
  double block_configured;
  int block_iops;
  double block_used;
  double ephemeral_configured;
  int ephemeral_iops;
  double ephemeral_used;
  char * vm_id;
} MEASUREMENT_FSYS_USE;

/**************************************************************************//**
 * Latency Bucket.
 * JSON equivalent field: latencyBucketMeasure
 *****************************************************************************/
typedef struct measurement_latency_bucket {
  double low_end;
  double high_end;
  int count;
} MEASUREMENT_LATENCY_BUCKET;

/**************************************************************************//**
 * Virtual NIC usage.
 * JSON equivalent field: vNicUsage
 *****************************************************************************/
typedef struct measurement_vnic_use {
  int broadcast_packets_in;
  int broadcast_packets_out;
  int bytes_in;
  int bytes_out;
  int multicast_packets_in;
  int multicast_packets_out;
  int unicast_packets_in;
  int unicast_packets_out;
  char * vnic_id;
} MEASUREMENT_VNIC_USE;

/**************************************************************************//**
 * Feature Usage.
 * JSON equivalent field: featuresInUse
 *****************************************************************************/
typedef struct measurement_feature_use {
  char * feature_id;
  double feature_utilization;
} MEASUREMENT_FEATURE_USE;

/**************************************************************************//**
 * Measurement Group.
 * JSON equivalent field: measurementGroup
 *****************************************************************************/
typedef struct measurement_group {
  char * name;
  DLIST measurements;
} MEASUREMENT_GROUP;

/**************************************************************************//**
 * Custom Defined Measurement.
 * JSON equivalent field: measurements
 *****************************************************************************/
typedef struct custom_measurement {
  char * name;
  char * value;
} CUSTOM_MEASUREMENT;

/**************************************************************************//**
 * Codec Usage.
 * JSON equivalent field: codecsInUse
 *****************************************************************************/
typedef struct measurement_codec_use {
  char * codec_id;
  int codec_utilization;
} MEASUREMENT_CODEC_USE;

/**************************************************************************//**
 * Report.
 * JSON equivalent field: measurementsForVfReportingFields
 *
 * @note  This is an experimental event type and is not currently a formal part
 *        of AT&T's specification.
 *****************************************************************************/
typedef struct event_report {
  EVENT_HEADER header;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  double measurement_interval;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST feature_usage;
  DLIST measurement_groups;
  int measurement_fields_version;

} EVENT_REPORT;

/**************************************************************************//**
 * Library initialization.
 *
 * Initialize the EVEL library.
 *
 * @note  This function initializes the cURL library.  Applications making use
 *        of libcurl may need to pull the initialization out of here.  Note
 *        also that this function is not threadsafe as a result - refer to
 *        libcurl's API documentation for relevant warnings.
 *
 * @sa  Matching Term function.
 *
 * @param   fqdn    The API's FQDN or IP address.
 * @param   port    The API's port.
 * @param   path    The optional path (may be NULL).
 * @param   topic   The optional topic part of the URL (may be NULL).
 * @param   secure  Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param   username  Username for Basic Authentication of requests.
 * @param   password  Password for Basic Authentication of requests.
 * @param   source_type The kind of node we represent.
 * @param   role    The role this node undertakes.
 * @param   verbosity  0 for normal operation, positive values for chattier
 *                        logs.
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS      On success
 * @retval  ::EVEL_ERR_CODES  On failure.
 *****************************************************************************/
EVEL_ERR_CODES evel_initialize(const char const * fqdn,
                               int port,
                               const char const * path,
                               const char const * topic,
                               int secure,
                               const char const * username,
                               const char const * password,
                               EVEL_SOURCE_TYPES source_type,
                               const char const *role,
                               int verbosity
                               );

/**************************************************************************//**
 * Clean up the EVEL library.
 *
 * @note that at present don't expect Init/Term cycling not to leak memory!
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS On success
 * @retval  "One of ::EVEL_ERR_CODES" On failure.
 *****************************************************************************/
EVEL_ERR_CODES evel_terminate(void);

EVEL_ERR_CODES evel_post_event(EVENT_HEADER * event);
const char const * evel_error_string(void);


/**************************************************************************//**
 * Free an event.
 *
 * Free off the event supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note  It is safe to free a NULL pointer.
 *****************************************************************************/
 void evel_free_event(void * event);

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
                                   EVENT_HEADER * event);

/**************************************************************************//**
 * Callback function to provide returned data.
 *
 * Copy data into the supplied buffer, write_callback::ptr, checking size
 * limits.
 *
 * @returns   Number of bytes placed into write_callback::ptr. 0 for EOF.
 *****************************************************************************/
size_t evel_write_callback(void *contents,
                           size_t size,
                           size_t nmemb,
                           void *userp);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   HEARTBEAT - (includes common header, too)                               */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new heartbeat event.
 *
 * @note that the heartbeat is just a "naked" commonEventHeader!
 *
 * @returns pointer to the newly manufactured ::EVENT_HEADER.  If the event is
 *          not used it must be released using ::evel_free_event
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEADER * evel_new_heartbeat();

/**************************************************************************//**
 * Free an event header.
 *
 * Free off the event header supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the header itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_header(EVENT_HEADER * event);

/**************************************************************************//**
 * Initialize a newly created event header.
 *
 * @param header  Pointer to the header being initialized.
 *****************************************************************************/
void evel_init_header(EVENT_HEADER * header);

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
                            EVENT_HEADER * event);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   FAULT                                                                   */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new fault event.
 *
 *
 * @returns pointer to the newly manufactured ::EVENT_FAULT.  If the event is
 *          not used it must be released using ::evel_free_event
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_FAULT * evel_new_fault(const char const * condition,
                             const char const * specific_problem,
                             EVEL_EVENT_PRIORITIES priority,
                             EVEL_FAULT_SEVERITIES severity);

/**************************************************************************//**
 * Free an Fault.
 *
 * Free off the Fault supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the Fault itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_fault(EVENT_FAULT * event);


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
 *                   returns
 *****************************************************************************/
void evel_fault_interface_set(EVENT_FAULT * fault,
                              const char const * interface);

/**************************************************************************//**
 * Add an additional value name/value pair to the Fault.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param fault     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.
 * @param value     ASCIIZ string with the attribute's value.
 *****************************************************************************/
void evel_fault_addl_info_add(EVENT_FAULT * fault, char * name, char * value);

/**************************************************************************//**
 * Set the Event Type property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param type       The Event Typeto be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns
 *****************************************************************************/
void evel_fault_type_set(EVENT_FAULT * fault, const char const * type);

/**************************************************************************//**
 * Encode the fault as a JSON fault.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_fault::json.
 * @param event     Pointer to the ::EVENT_FAULT to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_fault(char * json,
                           int max_size,
                           EVENT_FAULT * event);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   MEASUREMENT                                                             */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

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
                                         int request_rate);

/**************************************************************************//**
 * Free a Measurement.
 *
 * Free off the Measurement supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the Fault itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement(EVENT_MEASUREMENT * event);

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
                               const char const * type);

/**************************************************************************//**
 * Add an additional CPU usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param name      ASCIIZ string with the attribute's name.
 * @param value     ASCIIZ string with the attribute's value.
 *****************************************************************************/
void evel_measurement_cpu_use_add(EVENT_MEASUREMENT * measurement,
                                 char * name, double value);

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
                                 int ephemeral_iops);

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
                                 int count);

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
                                   int unicast_packets_out);

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
                                      double utilization);

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the Measurement.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param name     ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_measurement_custom_measurement_add(EVENT_MEASUREMENT * measurement,
                                             const char const * group,
                                             const char const * name,
                                             const char const * value);

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
                                    int utilization);

/**************************************************************************//**
}
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
                                      double cpu_use);

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
                                         int media_ports_in_use);

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
                                              double scaling_metric);

/**************************************************************************//**
 * Encode the fault as a JSON fault.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_fault::json.
 * @param event     Pointer to the ::EVENT_FAULT to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_measurement(char * json,
                                 int max_size,
                                 EVENT_MEASUREMENT * event);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   REPORT                                                                  */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

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
EVENT_REPORT * evel_new_report(double measurement_interval);

/**************************************************************************//**
 * Free a Report.
 *
 * Free off the Report supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the Report itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_report(EVENT_REPORT * event);

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
void evel_report_type_set(EVENT_REPORT * report, const char const * type);

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
                                 double utilization);

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
                                        const char const * value);

/**************************************************************************//**
 * Encode the report as a JSON report.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in evel_json_encode_report::json.
 * @param event     Pointer to the ::EVENT_REPORT to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_report(char * json, int max_size, EVENT_REPORT * event);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   LOGGING                                                                 */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/* Debug macros.                                                             */
/*****************************************************************************/
#define EVEL_DEBUG(FMT, ...)   log_debug(EVEL_LOG_DEBUG, (FMT), ##__VA_ARGS__)
#define EVEL_INFO(FMT, ...)    log_debug(EVEL_LOG_INFO, (FMT), ##__VA_ARGS__)
#define EVEL_SPAMMY(FMT, ...)  log_debug(EVEL_LOG_SPAMMY, (FMT), ##__VA_ARGS__)
#define EVEL_ERROR(FMT, ...)   log_debug(EVEL_LOG_ERROR, "ERROR: " FMT, \
                                         ##__VA_ARGS__)
#define EVEL_ENTER()                                                          \
        {                                                                     \
          log_debug(EVEL_LOG_DEBUG, "Enter %s {", __FUNCTION__);              \
          debug_indent += 2;                                                  \
        }
#define EVEL_EXIT()                                                           \
        {                                                                     \
          debug_indent -= 2;                                                  \
          log_debug(EVEL_LOG_DEBUG, "Exit %s }", __FUNCTION__);               \
        }

#define INDENT_SEPARATORS                                                     \
        "| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | "

extern EVEL_LOG_LEVELS debug_level;
extern int debug_indent;
extern FILE * fout;

/**************************************************************************//**
 * Initialize logging
 *
 * @param[in] level  The debugging level - one of ::EVEL_LOG_LEVELS
 * @param[in] ident  The identifier for our logs.
 *****************************************************************************/
void log_initialize(EVEL_LOG_LEVELS level, const char * ident);

/**************************************************************************//**
 * Log debug information
 *
 * Logs debugging information in a platform independent manner.
 *
 * @param[in] level   The debugging level - one of ::EVEL_LOG_LEVELS
 * @param[in] format  Log formatting string in printf format.
 * @param[in] ...     Variable argument list.
 *****************************************************************************/
void log_debug(EVEL_LOG_LEVELS level, char * format, ...);

/***************************************************************************//*
 * Store the formatted string into the static error string and log the error.
 *
 * @param format  Error string in standard printf format.
 * @param ...     Variable parameters to be substituted into the format string.
 *****************************************************************************/
void log_error_state(char * format, ...);

#endif
