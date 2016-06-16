/**************************************************************************//**
 * @file
 * Utility providing example use of the ECOMP Vendor Event Listener API.
 *
 * This software is intended to show the essential elements of the library's
 * use.
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
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:  This product includes software
 * developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
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
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/signal.h>
#include <pthread.h>
#include <mcheck.h>

#include "evel.h"
#include "evel_demo.h"

/**************************************************************************//**
 * Definition of long options to the program.
 *
 * See the documentation for getopt_long() for details of the structure's use.
 *****************************************************************************/
static const struct option long_options[] = {
    {"help",    no_argument,       0, 'h'},
    {"fqdn",    required_argument, 0, 'f'},
    {"port",    required_argument, 0, 'n'},
    {"path",    required_argument, 0, 'p'},
    {"topic",   required_argument, 0, 't'},
    {"https",   no_argument,       0, 's'},
    {"verbose", no_argument,       0, 'v'},
    {"cycles",  required_argument, 0, 'c'},
    {0, 0, 0, 0}
  };

/**************************************************************************//**
 * Definition of short options to the program.
 *****************************************************************************/
static const char* short_options = "hf:n:p:t:sc:";

/**************************************************************************//**
 * Basic user help text describing the usage of the application.
 *****************************************************************************/
static const char* usage_text =
"evel_demo [--help]\n"
"          --fqdn <domain>\n"
"          --port <port_number>\n"
"          [--path <path>]\n"
"          [--topic <topic>]\n"
"          [--https]\n"
"          [--cycles <cycles>]\n"
"\n"
"Demonstrate use of the ECOMP Vendor Event Listener API.\n"
"\n"
"  -h         Display this usage message.\n"
"  --help\n"
"\n"
"  -f         The FQDN or IP address to the RESTful API.\n"
"  --fqdn\n"
"\n"
"  -n         The port number the RESTful API.\n"
"  --port\n"
"\n"
"  -p         The optional path prefix to the RESTful API.\n"
"  --path\n"
"\n"
"  -t         The optional topic part of the RESTful API.\n"
"  --topic\n"
"\n"
"  -s         Use HTTPS rather than HTTP for the transport.\n"
"  --https\n"
"\n"
"  -c         Loop <cycles> times round the main loop.  Default = 1.\n"
"  --cycles\n"
"\n"
"  -v         Generate much chattier logs.\n"
"  --verbose\n";

/**************************************************************************//**
 * Global flag to initiate shutdown.
 *****************************************************************************/
static int glob_exit_now = 0;

static void show_usage(FILE* fp)
{
  fputs(usage_text, fp);
}


/**************************************************************************//**
 * Main function.
 *
 * Parses the command-line then ...
 *
 * @param[in] argc  Argument count.
 * @param[in] argv  Argument vector - for usage see usage_text.
 *****************************************************************************/
int main(int argc, char ** argv)
{
  sigset_t sig_set;
  pthread_t thread_id;
  int option_index = 0;
  int param = 0;
  char * api_fqdn = NULL;
  int api_port = 0;
  char * api_path = NULL;
  char * api_topic = NULL;
  int api_secure = 0;
  int verbose_mode = 0;
  int cycles = 1;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_HEADER * heartbeat = NULL;
  EVENT_FAULT * fault = NULL;
  EVENT_MEASUREMENT * measurement = NULL;
  EVENT_REPORT * report = NULL;

  /***************************************************************************/
  /* We're very interested in memory management problems so check behavior.  */
  /***************************************************************************/
  mcheck(NULL);

  if (argc < 2)
  {
    show_usage(stderr);
    exit(-1);
  }
  param = getopt_long(argc, argv,
                      short_options,
                      long_options,
                      &option_index);
  while (param != -1)
  {
    switch (param)
    {
      case 'h':
        show_usage(stdout);
        exit(0);
        break;

      case 'f':
        api_fqdn = optarg;
        break;

      case 'n':
        api_port = atoi(optarg);
        break;

      case 'p':
        api_path = optarg;
        break;

      case 't':
        api_topic = optarg;
        break;

      case 's':
        api_secure = 1;
        break;

      case 'c':
        cycles = atoi(optarg);
        break;

      case 'v':
        verbose_mode = 1;
        break;

      case '?':
        /*********************************************************************/
        /* Unrecognized parameter - getopt_long already printed an error     */
        /* message.                                                          */
        /*********************************************************************/
        break;

      default:
        fprintf(stderr, "Code error: recognized but missing option (%d)!\n",
                param);
        exit(-1);
    }

    /*************************************************************************/
    /* Extract next parameter.                                               */
    /*************************************************************************/
    param = getopt_long(argc, argv,
                        short_options,
                        long_options,
                        &option_index);
  }

  /***************************************************************************/
  /* All the command-line has parsed cleanly, so now check that the options  */
  /* are meaningful.                                                         */
  /***************************************************************************/
  if (api_fqdn == NULL)
  {
    fprintf(stderr, "FQDN of the Vendor Event Listener API server must be "
                    "specified.\n");
    exit(1);
  }
  if (api_port <= 0 || api_port > 65535)
  {
    fprintf(stderr, "Port for the Vendor Event Listener API server must be "
                    "specified between 1 and 65535.\n");
    exit(1);
  }
  if (cycles <= 0)
  {
    fprintf(stderr, "Number of cycles around the main loop must be an"
                    "integer greater than zero.\n");
    exit(1);
  }

  /***************************************************************************/
  /* Set up default signal behaviour.  Block all signals we trap explicitly  */
  /* on the signal_watcher thread.                                           */
  /***************************************************************************/
  sigemptyset(&sig_set);
  sigaddset(&sig_set, SIGALRM);
  sigaddset(&sig_set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &sig_set, NULL);

  /***************************************************************************/
  /* Start the signal watcher thread.                                        */
  /***************************************************************************/
  if (pthread_create(&thread_id, NULL, signal_watcher, &sig_set) != 0)
  {
    fprintf(stderr, "Failed to start signal watcher thread.");
    exit(1);
  }
  pthread_detach(thread_id);

  /***************************************************************************/
  /* Version info                                                            */
  /***************************************************************************/
  printf("%s built %s %s\n", argv[0], __DATE__, __TIME__);

  /***************************************************************************/
  /* Initialize the EVEL interface.                                          */
  /***************************************************************************/
  if (evel_initialize(api_fqdn,
                      api_port,
                      api_path,
                      api_topic,
                      api_secure,
                      "",
                      "",
                      EVEL_SOURCE_VIRTUAL_MACHINE,
                      "EVEL demo client",
                      verbose_mode))
  {
    fprintf(stderr, "Failed to initialize the EVEL library!!!");
    exit(-1);
  }
  else
  {
    EVEL_INFO("Initialization completed");
  }

  /***************************************************************************/
  /* MAIN LOOP                                                               */
  /***************************************************************************/
  printf("Starting %d loops...\n", cycles);
  while (cycles > 0)
  {
    EVEL_INFO("MAI: Starting main loop");
    printf("Starting main loop\n");

    heartbeat = evel_new_heartbeat();
    if (heartbeat != NULL)
    {
      evel_rc = evel_post_event(heartbeat);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New heartbeat failed");
    }

    fault = evel_new_fault("My alarm condition",
                           "It broke very badly",
                           EVEL_PRIORITY_NORMAL,
                           EVEL_SEVERITY_MAJOR);
    if (fault != NULL)
    {
      evel_fault_type_set(fault, "Bad things happen...");
      evel_fault_interface_set(fault, "My Interface Card");
      evel_fault_addl_info_add(fault, "name1", "value1");
      evel_fault_addl_info_add(fault, "name2", "value2");
      evel_rc = evel_post_event((EVENT_HEADER *)fault);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New fault failed");
    }
    printf("   Processed heartbeat\n");

    measurement = evel_new_measurement(1,2,3.3,4.4,5.5,6.6,7);
    if (measurement != NULL)
    {
      evel_measurement_type_set(measurement, "Perf management...");
      evel_measurement_agg_cpu_use_set(measurement, 8.8);
      evel_measurement_cpu_use_add(measurement, "cpu1", 11.11);
      evel_measurement_cpu_use_add(measurement, "cpu2", 22.22);
      evel_measurement_fsys_use_add(measurement,"00-11-22",100.11, 100.22, 33,
                                                           200.11, 200.22, 44);
      evel_measurement_fsys_use_add(measurement,"33-44-55",300.11, 300.22, 55,
                                                           400.11, 400.22, 66);

      evel_measurement_latency_add(measurement, 0.0, 10.0, 20);
      evel_measurement_latency_add(measurement, 10.0, 20.0, 30);

      evel_measurement_vnic_use_add(measurement, "eth0", 1, 2,
                                                         3, 4,
                                                         5, 6,
                                                         7, 8);
      evel_measurement_vnic_use_add(measurement, "eth1", 11, 12,
                                                         13, 14,
                                                         15, 16,
                                                         17, 18);

      evel_measurement_feature_use_add(measurement, "FeatureA", 123.4);
      evel_measurement_feature_use_add(measurement, "FeatureB", 567.8);

      evel_measurement_codec_use_add(measurement, "G711a", 91);
      evel_measurement_codec_use_add(measurement, "G729ab", 92);

      evel_measurement_media_port_use_set(measurement, 1234);

      evel_measurement_vnfc_scaling_metric_set(measurement, 1234.5678);

      evel_measurement_custom_measurement_add(measurement,
                                              "Group1", "Name1", "Value1");
      evel_measurement_custom_measurement_add(measurement,
                                              "Group2", "Name1", "Value1");
      evel_measurement_custom_measurement_add(measurement,
                                              "Group2", "Name2", "Value2");

      evel_rc = evel_post_event((EVENT_HEADER *)measurement);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post Measurement failed %d (%s)",
                    evel_rc,
                    evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New Measurement failed");
    }
    printf("   Processed measurement\n");

    report = evel_new_report(1.1);
    if (report != NULL)
    {
      evel_report_type_set(report, "Perf reporting...");
      evel_report_feature_use_add(report, "FeatureA", 123.4);
      evel_report_feature_use_add(report, "FeatureB", 567.8);
      evel_report_custom_measurement_add(report, "Group1", "Name1", "Value1");
      evel_report_custom_measurement_add(report, "Group2", "Name1", "Value1");
      evel_report_custom_measurement_add(report, "Group2", "Name2", "Value2");

      evel_rc = evel_post_event((EVENT_HEADER *)report);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post of Report failed %d (%s)",
                    evel_rc,
                    evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New Report failed");
    }
    printf("   Processed Report\n");

    /*************************************************************************/
    /* MAIN RETRY LOOP.  Loop every 10 secs unless the final pass.           */
    /*************************************************************************/
    fflush(stdout);
    if (--cycles > 0)
    {
      sleep(10);
    }
 }
  /***************************************************************************/
  /* We are exiting, but allow the final set of events to be dispatched      */
  /* properly first.                                                         */
  /***************************************************************************/
  sleep(1);
  printf("All done - exiting!\n");
  return 0;
}

/**************************************************************************//**
 * Signal watcher.
 *
 * Signal catcher for incoming signal processing.  Work out which signal has
 * been received and process it accordingly.
 *
 * param[in]  void_sig_set  The signal mask to listen for.
 *****************************************************************************/
void *signal_watcher(void *void_sig_set)
{
  sigset_t *sig_set = (sigset_t *)void_sig_set;
  int sig = 0;
  int old_type = 0;
  siginfo_t sig_info;

  /***************************************************************************/
  /* Set this thread to be cancellable immediately.                          */
  /***************************************************************************/
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);

  while (!glob_exit_now)
  {
    /*************************************************************************/
    /* Wait for a signal to be received.                                     */
    /*************************************************************************/
    sig = sigwaitinfo(sig_set, &sig_info);
    switch (sig)
    {
      case SIGALRM:
        /*********************************************************************/
        /* Failed to do something in the given amount of time.  Exit.        */
        /*********************************************************************/
        EVEL_ERROR( "Timeout alarm");
        fprintf(stderr,"Timeout alarm - quitting!\n");
        exit(2);
        break;

      case SIGINT:
        EVEL_INFO( "Interrupted - quitting");
        printf("\n\nInterrupted - quitting!\n");
        glob_exit_now = 1;
        break;
    }
  }

  evel_terminate();
  exit(0);
  return(NULL);
}
