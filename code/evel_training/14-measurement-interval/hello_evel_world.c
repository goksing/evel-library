#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "evel.h"
#include "evel_test_control.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_heartbeat();
static void demo_measurement(const int interval);

unsigned long long epoch_start = 0;

int main(int argc, char ** argv)
{
  printf("\nHello AT&T Vendor Event world!\n");
  fflush(stdout);

  if (argc != 5)
  {
    fprintf(stderr,
            "Usage: %s <FQDN>|<IP address> <port> "
            "<username> <password>\n", argv[0]);
    exit(-1);
  }

  char * api_fqdn = argv[1];
  int api_port = atoi(argv[2]);
  int api_secure = 0;

  /***************************************************************************/
  /* Initialize                                                              */
  /***************************************************************************/
  if (evel_initialize(api_fqdn,                     /* FQDN                  */
                      api_port,                     /* Port                  */
                      NULL,                         /* optional path         */
                      NULL,                         /* optional topic        */
                      api_secure,                   /* HTTPS?                */
                      argv[3],                      /* Username              */
                      argv[4],                      /* Password              */
                      EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type           */
                      "EVEL training demo",         /* Role                  */
                      0))                           /* Verbosity             */
  {
    fprintf(stderr, "\nFailed to initialize the EVEL library!!!\n");
    exit(-1);
  }
  else
  {
    printf("\nInitialization completed\n");
  }

  /***************************************************************************/
  /* Work out a start time for measurements, and sleep for initial period.   */
  /***************************************************************************/
  int sleep_time = 1;
  struct timeval tv_start;
  gettimeofday(&tv_start, NULL);
  epoch_start = tv_start.tv_usec + 1000000 * tv_start.tv_sec;
  sleep(sleep_time);

  int index;
  for (index = 0; index < 10; index++)
  {
    /*************************************************************************/
    /* On the second and fifth iteration, get the test_collector to change   */
    /* the interval.                                                         */
    /*************************************************************************/
    if (index == 2)
    {
      printf("TestControl: measurement interval -> 2s\n");
      evel_test_control_meas_interval(2, api_secure, api_fqdn, api_port);
    }
    if (index == 5)
    {
      printf("TestControl: measurement interval -> 5s\n");
      evel_test_control_meas_interval(5, api_secure, api_fqdn, api_port);
    }

    /*************************************************************************/
    /* Heartbeat to pick up the suppression change.                          */
    /*************************************************************************/
    demo_heartbeat();

    /*************************************************************************/
    /* Raise a measurement and sleep.                                        */
    /*************************************************************************/
    printf("Report measurements\n");
    fflush(stdout);
    demo_measurement(sleep_time);
    printf("Sleeping for %ds\n", sleep_time);
    fflush(stdout);
    sleep(sleep_time);

    /*************************************************************************/
    /* Update measurement interval.                                          */
    /*************************************************************************/
    int measurement_interval;
    measurement_interval = evel_get_measurement_interval();
    if (measurement_interval == EVEL_MEASUREMENT_INTERVAL_UKNOWN)
    {
      sleep_time = 1;
    }
    else
    {
      sleep_time = measurement_interval;
    }
    printf("EVEL measurement interval = %d\n\n", measurement_interval);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send a heatbeat.
 *****************************************************************************/
void demo_heartbeat()
{
  EVENT_HEADER * heartbeat = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  heartbeat = evel_new_heartbeat();
  if (heartbeat != NULL)
  {
    evel_rc = evel_post_event(heartbeat);
    if (evel_rc != EVEL_SUCCESS)
    {
      printf("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    printf("New heartbeat failed");
  }
}

/**************************************************************************//**
 * Create and send a measurement event.
 *****************************************************************************/
void demo_measurement(const int interval)
{
  EVENT_MEASUREMENT * measurement = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Measurement                                                             */
  /***************************************************************************/
  measurement = evel_new_measurement(interval);
  if (measurement != NULL)
  {
    printf("New measurement created...\n");
    evel_measurement_type_set(measurement, "Perf management...");
    evel_measurement_conc_sess_set(measurement, 1);
    evel_measurement_cfg_ents_set(measurement, 2);
    evel_measurement_mean_req_lat_set(measurement, 4.4);
    evel_measurement_mem_cfg_set(measurement, 6.6);
    evel_measurement_mem_used_set(measurement, 3.3);
    evel_measurement_request_rate_set(measurement, 6);
    evel_measurement_agg_cpu_use_set(measurement, 8.8);
    evel_measurement_cpu_use_add(measurement, "cpu1", 11.11);
    evel_measurement_cpu_use_add(measurement, "cpu2", 22.22);
    evel_measurement_fsys_use_add(measurement,"00-11-22",100.11, 100.22, 33,
                                  200.11, 200.22, 44);
    evel_measurement_fsys_use_add(measurement,"33-44-55",300.11, 300.22, 55,
                                  400.11, 400.22, 66);

    bucket = evel_new_meas_latency_bucket(20);
    evel_meas_latency_bucket_low_end_set(bucket, 0.0);
    evel_meas_latency_bucket_high_end_set(bucket, 10.0);
    evel_meas_latency_bucket_add(measurement, bucket);

    bucket = evel_new_meas_latency_bucket(30);
    evel_meas_latency_bucket_low_end_set(bucket, 10.0);
    evel_meas_latency_bucket_high_end_set(bucket, 20.0);
    evel_meas_latency_bucket_add(measurement, bucket);

    vnic_use = evel_new_measurement_vnic_use("eth0", 100, 200, 3, 4);
    evel_vnic_use_bcast_pkt_in_set(vnic_use, 1);
    evel_vnic_use_bcast_pkt_out_set(vnic_use, 2);
    evel_vnic_use_mcast_pkt_in_set(vnic_use, 5);
    evel_vnic_use_mcast_pkt_out_set(vnic_use, 6);
    evel_vnic_use_ucast_pkt_in_set(vnic_use, 7);
    evel_vnic_use_ucast_pkt_out_set(vnic_use, 8);
    evel_meas_vnic_use_add(measurement, vnic_use);

    vnic_use = evel_new_measurement_vnic_use("eth1", 110, 240, 13, 14);
    evel_vnic_use_bcast_pkt_in_set(vnic_use, 11);
    evel_vnic_use_bcast_pkt_out_set(vnic_use, 12);
    evel_vnic_use_mcast_pkt_in_set(vnic_use, 15);
    evel_vnic_use_mcast_pkt_out_set(vnic_use, 16);
    evel_vnic_use_ucast_pkt_in_set(vnic_use, 17);
    evel_vnic_use_ucast_pkt_out_set(vnic_use, 18);
    evel_meas_vnic_use_add(measurement, vnic_use);

    evel_measurement_errors_set(measurement, 1, 0, 2, 1);

    evel_measurement_feature_use_add(measurement, "FeatureA", 123);
    evel_measurement_feature_use_add(measurement, "FeatureB", 567);

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

    /*************************************************************************/
    /* Work out the time, to use as end of measurement period.               */
    /*************************************************************************/
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    unsigned long long epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;
    evel_start_epoch_set(&measurement->header, epoch_start);
    evel_last_epoch_set(&measurement->header, epoch_now);
    epoch_start = epoch_now;
    evel_reporting_entity_name_set(&measurement->header, "measurer");
    evel_reporting_entity_id_set(&measurement->header, "measurer_id");

    evel_rc = evel_post_event((EVENT_HEADER *)measurement);
    if (evel_rc == EVEL_SUCCESS)
    {
      printf("Post OK!\n");
    }
    else
    {
      printf("Post Failed %d (%s)\n", evel_rc, evel_error_string());
    }
  }
  else
  {
    printf("Failed to create event (%s)\n", evel_error_string());
  }

  printf("   Processed Measurement\n");
}
