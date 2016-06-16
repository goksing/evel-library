#ifndef EVEL_INTERNAL_INCLUDED
#define EVEL_INTERNAL_INCLUDED

/**************************************************************************//**
 * @file
 * EVEL internal definitions.
 *
 * These are internal definitions which need to be shared between modules
 * within the library but are not intended for external consumption.
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

/*****************************************************************************/
/* Define some type-safe min/max macros.                                     */
/*****************************************************************************/
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

/**************************************************************************//**
 * The Functional Role of the equipment represented by this VNF.
 *****************************************************************************/
extern char * functional_role;

/**************************************************************************//**
 * The type of equipment represented by this VNF.
 *****************************************************************************/
extern EVEL_SOURCE_TYPES event_source_type;

/**************************************************************************//**
 * A chunk of memory used in the cURL functions.
*****************************************************************************/
typedef struct memory_chunk {
  char *memory;
  size_t size;
} MEMORY_CHUNK;

/**************************************************************************//**
 * Global commands that may be sent to the Event Handler thread.
 *****************************************************************************/
typedef enum {
  EVT_CMD_TERMINATE,
  EVT_CMD_MAX_COMMANDS
} EVT_HANDLER_COMMAND;

/**************************************************************************//**
 * State of the Event Handler thread.
 *****************************************************************************/
typedef enum {
  EVT_HANDLER_UNINITIALIZED,      /** The library cannot handle events.      */
  EVT_HANDLER_INACTIVE,           /** The event handler thread not started.  */
  EVT_HANDLER_ACTIVE,             /** The event handler thread is started.   */
  EVT_HANDLER_REQUEST_TERMINATE,  /** Initial stages of shutdown.            */
  EVT_HANDLER_TERMINATING,        /** The ring-buffer is being depleted.     */
  EVT_HANDLER_TERMINATED,         /** The library is exited.                 */
  EVT_HANDLER_MAX_STATES          /** Maximum number of valid states.        */
} EVT_HANDLER_STATE;

/**************************************************************************//**
 * Internal event.
 * Pseudo-event used for routing internal commands.
 *****************************************************************************/
typedef struct event_internal {
  EVENT_HEADER header;
  EVT_HANDLER_COMMAND command;
} EVENT_INTERNAL;

/**************************************************************************//**
 * Initialize the event handler.
 *
 * Primarily responsible for getting cURL ready for use.
 *
 * @param[in] api_url   The URL where the Vendor Event Listener API is expected
 *                      to be.
 * @param[in] username  The username for the Basic Authentication of requests.
 * @param[in] password  The password for the Basic Authentication of requests.
 * @param     verbosity 0 for normal operation, positive values for chattier
 *                        logs.
 *****************************************************************************/
EVEL_ERR_CODES event_handler_initialize(const char const *api_url,
                                        const char const * username,
                                        const char const * password,
                                        int verbosity);

/**************************************************************************//**
 * Terminate the event handler.
 *
 * Shuts down the event handler thread in as clean a way as possible. Sets the
 * global exit flag and then signals the thread to interrupt it since it's
 * most likely waiting on the ring-buffer.
 *
 * Having achieved an orderly shutdown of the event handler thread, clean up
 * the cURL library's resources cleanly.
 *
 *  @return Status code.
 *  @retval ::EVEL_SUCCESS if everything OK.
 *  @retval One of ::EVEL_ERR_CODES if there was a problem.
 *****************************************************************************/
EVEL_ERR_CODES event_handler_terminate();

/**************************************************************************//**
 * Run the event handler.
 *
 * Spawns the thread responsible for handling events and sending them to the
 * API.
 *
 *  @return Status code.
 *  @retval ::EVEL_SUCCESS if everything OK.
 *  @retval One of ::EVEL_ERR_CODES if there was a problem.
 *****************************************************************************/
 EVEL_ERR_CODES event_handler_run();

 /**************************************************************************//**
 * Create a new internal event.
 *
 * @note    The mandatory fields on the Fault must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Fault has immutable properties.
 * @param   command   The condition indicated by the event.
 * @returns pointer to the newly manufactured ::EVENT_INTERNAL.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_INTERNAL * evel_new_internal_event(EVT_HANDLER_COMMAND command);

/*************************************************************************//**
 * Free an internal event.
 *
 * Free off the event supplied.  Will recursively free all the contained
 * allocated memory.
 *
 * @note It does not free the internal event itself, since that may be part of
 * a larger structure.
 *****************************************************************************/
void evel_free_internal_event(EVENT_INTERNAL * event);

#endif
