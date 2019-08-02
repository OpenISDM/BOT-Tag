/*
Copyright (c) 2016 Academia Sinica, Institute of Information Science

License:

    GPL 3.0 : The content of this file is subject to the terms and
    conditions defined in file 'COPYING.txt', which is part of this source
    code package.

Project Name:

    BeDIS

File Description:

    This header file contains declarations of variables, structs and
    functions and definitions of global variables used in the LBeacon.c file.

File Name:

    Tag.h

Version:

    1.0,  20190429

Abstract:

Authors:

    Chun Yu Lai, chunyu1202@gmail.com

*/

#ifndef TAG_H
#define TAG_H

/*
* INCLUDES
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/file.h>

#include "zlog.h"
#include "Version.h"

/*
  CONSTANTS
*/

/* File path of the config file of the Tag */
#define CONFIG_FILE_NAME "/home/pi/Tag/config/config.conf"

/* File path of the logging file*/
#define LOG_FILE_NAME "/home/pi/Tag/config/zlog.conf"

/* The category defined of log file used for health report */
#define LOG_CATEGORY_HEALTH_REPORT "Health_Report"

/* The category defined for the printf during debugging */
#define LOG_CATEGORY_DEBUG "Tag_Debug"

/* The lock file for Tag  */
#define TAG_LOCK_FILE "/home/pi/Tag/bin/Tag.pid"

/* Number of times to retry open file, because file openning operation may have
   transient failure. */
#define FILE_OPEN_RETRY 5

/* Number of times to retry getting a dongle, because this operation may have
   transient failure. */
#define DONGLE_GET_RETRY 5

/* Number of times to retry opening socket, because socket openning operation
   may have transient failure. */
#define SOCKET_OPEN_RETRY 5

/* Maximum number of characters in each line of config file */
#define CONFIG_BUFFER_SIZE 64

/* Parameter that marks the start of the config file */
#define DELIMITER "="

/* For following EIR_ constants, please refer to Bluetooth specifications for
the defined values.
https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
*/
/* BlueZ bluetooth extended inquiry response protocol: flags */
#define EIR_FLAGS 0X01

/* BlueZ bluetooth extended inquiry response protocol: short local name */
#define EIR_NAME_SHORT 0x08

/* BlueZ bluetooth extended inquiry response protocol: complete local name */
#define EIR_NAME_COMPLETE 0x09

/* BlueZ bluetooth extended inquiry response protocol: Manufacturer Specific
   Data */
#define EIR_MANUFACTURE_SPECIFIC_DATA 0xFF

/* Timeout in milliseconds of hci_send_req funtion */
#define HCI_SEND_REQUEST_TIMEOUT_IN_MS 1000

/* Number of characters in the name of a Bluetooth device */
#define LENGTH_OF_DEVICE_NAME 30

/* Number of characters in the uuid of a Bluetooth device */
#define LENGTH_OF_UUID 33

/* Number of characters in a Bluetooth MAC address */
#define LENGTH_OF_MAC_ADDRESS 18

/* Time interval in micro seconds for busy-wait checking in threads */
#define INTERVAL_FOR_BUSY_WAITING_CHECK_IN_MICRO_SECONDS 500000


typedef enum _ErrorCode{

    WORK_SUCCESSFULLY = 0,
    E_OPEN_FILE = 1,
    E_OPEN_DEVICE = 2,
    E_OPEN_SOCKET = 3,
    E_ADVERTISE_STATUS = 4,
    E_ADVERTISE_MODE = 5,
    E_SEND_REQUEST_TIMEOUT = 6,
    
    MAX_ERROR_CODE

} ErrorCode;

/*
  TYPEDEF STRUCTS
*/

/* The configuration file structure */

typedef struct Config {
    int advertise_dongle_id;

    /* Time interval in units of 0.625ms between advertising by a LBeacon */
    int advertise_interval_in_units_0625_ms;
    
    /* The rssi value used to advertise */
    int advertise_rssi_value;
   
} Config;


/* A global flag that is initially set to true by the main thread. It is set
   to false by any thread when the thread encounters a fatal error,
   indicating that it is about to exit. In addition, if user presses Ctrl+C,
   the ready_to_work will be set as false to stop all threadts. */
bool ready_to_work;

/* The pointer to the category of the log file */
zlog_category_t *category_health_report, *category_debug;


/*
  EXTERN STRUCTS
*/

/* In sys/poll.h, the struct for controlling the events. */
//extern struct pollfd;

/* In hci_sock.h, the struct for callback event from the socket. */
//extern struct hci_filter;

/*
  EXTERNAL GLOBAL VARIABLES
*/

extern int errno;

/*
  GLOBAL VARIABLES
*/

/* Struct for storing config information from the input file */
Config g_config;

/* UUID of LBeacon inside payload of advertising packet */
char lbeacon_uuid[LENGTH_OF_UUID];

/*
  FUNCTIONS
*/

/*
  single_running_instance:

      This function write a file lock to ensure that system has only one
      instance of running LBeacon.

  Parameters:
      file_name - the name of the lock file that specifies PID of running
                  LBeacon

  Return value:
      ErrorCode - indicate the result of execution, the expected return code
                  is WORK_SUCCESSFULLY

*/

ErrorCode single_running_instance(char *file_name);

/*
  get_config:

      This function reads the specified config file line by line until the
      end of file and copies the data in the lines into the Config struct
      global variable.

  Parameters:
      config - Pointer to config struct including file path, coordinates, etc.
      file_name - the name of the config file that stores all the beacon data

  Return value:

      ErrorCode - indicate the result of execution, the expected return code
                  is WORK_SUCCESSFULLY
*/

ErrorCode get_config(Config *config, char *file_name);

/*
  uuid_str_to_data:

     Convert uuid from string to unsigned integer.

  Parameters:

     uuid - The uuid in string type.

  Return value:

     unsigned int - The converted uuid in unsigned int type.
 */
unsigned int *uuid_str_to_data(char *uuid);


/*
  trim_string_tail:

     Trim the whitespace, newline and carry-return at the end of string when
     reading config messages.

  Parameters:

     message - the character array of input string

  Return value:

     None
 */
void trim_string_tail(char *message);

/*
  ctrlc_handler:

     If the user presses CTRL-C, the global variable ready_to_work will be set
     to false, and a signal will be thrown to stop running the program.

  Parameters:

     stop - A interger signal triggered by ctrl-c.

  Return value:

     None

 */
void ctrlc_handler(int stop);

/*
  enable_advertising:

      This function enables the LBeacon to start advertising, sets the time
      interval for advertising, and calibrates the RSSI value.

  Parameters:

      dongle_device_id - the bluetooth dongle device which the LBeacon uses
                         to advertise
      advertising_interval_in_units_0625_ms - the time interval in units of 0.625ms 
                                              during which the LBeacon can advertise
      advertising_uuid - universally unique identifier of advertiser
      major_number - major version number of LBeacon
      minor_number - minor version number of LBeacon
      rssi_value - RSSI value of the bluetooth device

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode enable_advertising(int dongle_device_id,
                             int advertising_interval_in_units_0625_ms,
                             char *advertising_uuid,
                             int major_number,
                             int minor_number,
                             int rssi_value);

/*
  disable_advertising:

      This function disables advertising of the beacon.

  Parameters:

      dongle_device_id - the bluetooth dongle device which the LBeacon needs to
                         disable advertising function

  Return value:

      ErrorCode - The error code for the corresponding error if the function
                  fails or WORK SUCCESSFULLY otherwise
*/

ErrorCode disable_advertising(int dongle_device_id);

/*
  EXTERNAL FUNCTIONS
*/

/*
  memset:

      This function is called to fill a block of memory.

  Parameters:

      ptr - the pointer points to the memory area
      value - the int value passed to the function which fills the blocks of
              memory using unsinged char convension of this value
      number - number of bytes in the memory area starting from ptr to be
               filled

  Return value:

      dst - a pointer to the memory area
*/

extern void * memset(void * ptr, int value, size_t number);

/*
  hci_open_dev:

      This function is called to open a Bluetooth socket with the specified
      resource number.

  Parameters:

      dev_id - the id of the Bluetooth socket device

  Return value:

      dd - device descriptor of the Bluetooth socket
*/

extern int hci_open_dev(int dev_id);

/*
  hci_filter_clear:

      This function is called to clear a specified filter.

  Parameters:

      f - the filter to be cleared

  Return value:

      None
*/

extern void hci_filter_clear(struct hci_filter *f);

/*
  hci_filter_set_ptype:

      This function is called to let filter set ptype.

  Parameters:

      t - the type
      f - the filter to be set

  Return value:

      None
*/

extern void hci_filter_set_ptype(int t, struct hci_filter *f);

/*
  hci_filter_set_event:

      This function is called to let filter set event

  Parameters:

      e - the event
      f - the filter to be set

  Return value:

      None
*/

extern void hci_filter_set_event(int e, struct hci_filter *f);

/*
  hci_write_inquiry_mode:

      This function is called to configure inquiry mode

  Parameters:

      dd - device descriptor of the open HCI socket
      mode - new inquiry mode
      to -

  Return value:

      None
*/

extern int hci_write_inquiry_mode(int dd, uint8_t mode, int to);

/*
  hci_send_cmd:

      This function is called to send cmd

  Parameters:

      dd - device descriptor of the open HCI socket
      ogf - opcode group field
      ocf - opcode command field
      plen - the length of the command parameters
      param - the parameters that function runs with

  Return value:

      0 for success. error number for error.
*/

extern int  hci_send_cmd(int dd, uint16_t ogf, uint16_t ocf, uint8_t plen,
                         void *param);

#endif
