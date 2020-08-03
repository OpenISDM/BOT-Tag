/*
  Copyright (c) BiDaE Technology Inc. All rights reserved.

  License:

    BiDaE SHAREWARE LICENSE
    Version 1.0, 31 July 2020

    Copyright (c) BiDaE Technology Inc. All rights reserved.
    The SOFTWARE implemented in the product is copyrighted and protected by 
    all applicable intellectual property laws worldwide. Any duplication of 
    the SOFTWARE other than for archiving or resetting purposes on the same 
    product without the written agreement from BiDaE Technology could be a 
    violation of law. For the avoidance of doubt, redistribution of this 
    SOFTWARE in source or binary form is prohibited. You agree to prevent 
    any unauthorized copying and redistribution of the SOFTWARE. 

    BiDaE Technology Inc. is the license steward. No one other than the 
    license steward has the right to modify or publish new versions of this 
    License. However, You may distribute the SOFTWARE under the terms of the 
    version of the License under which You originally received the Covered 
    Software, or under the terms of any subsequent version published by the 
    license steward.

    LIMITED WARRANTY:

    BiDaE Technology Inc. or its distributors, depending on which party sold 
    the SOFTWARE, warrants that the media on which the SOFTWARE is installed 
    will be free from defects in materials under normal and purposed use.

    BiDaE Technology Inc. or its distributor warrants, for your benefit alone, 
    that during the Warranty Period the SOFTWARE, shall operate substantially 
    in accordance with the functional specifications in the User's Manual. If, 
    during the Warranty Period, a defect in the SOFTWARE appears, You may 
    obtain a replacement of the SOFTWARE. Any replacement SOFTWARE will be 
    warranted for the remainder of the Warranty Period attached to the product.

    WHEN THE WARRANTY PERIOD HAS BEEN EXPIRED, THIS SOFTWARE IS PROVIDED 
    ''AS IS,'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
    PARTICULAR PURPOSE ARE DISCLAIMED. HENCEFORTH, IN NO EVENT SHALL BiDaE 
    TECHNOLOGY INC. OR ITS COLLABORATOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 Project Name:

      BeDIS

 File Description:

      This file contains the programs executed by location beacons to
      support indoor poositioning and object tracking functions.

 File Name:

      Tag.c

 Version:

       1.0,  20190802

 Abstract:

 Authors:

      Chun-Yu Lai, chunyu1202@gmail.com

*/

#include "Tag.h"
#include "zlog.h"

#define Debugging

ErrorCode single_running_instance(char *file_name){
    int retry_time = 0;
    int lock_file = 0;
    struct flock fl;

    retry_time = FILE_OPEN_RETRY;
    while(retry_time--){
        lock_file = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);

        if(-1 != lock_file){
            break;
        }
    }

    if(-1 == lock_file){
        zlog_error(category_health_report,
            "Unable to open lock file");
#ifdef Debugging
        zlog_error(category_debug,
            "Unable to open lock file");
#endif
        return E_OPEN_FILE;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if(fcntl(lock_file, F_SETLK, &fl) == -1){
        zlog_error(category_health_report, "Unable to lock file");
#ifdef Debugging
        zlog_error(category_debug, "Unable to lock file");
#endif
        close(lock_file);
        return E_OPEN_FILE;
    }

    char pids[10];
    snprintf(pids, sizeof(pids), "%d\n", getpid());
    if((size_t)write(lock_file, pids, strlen(pids)) != strlen(pids)){

        zlog_error(category_health_report,
                   "Unable to write pid into lock file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to write pid into lock file");
#endif
        close(lock_file);

        return E_OPEN_FILE;
    }

    return WORK_SUCCESSFULLY;
}


ErrorCode get_config(Config *config, char *file_name) {
    /* Return value is a struct containing all config information */
    int retry_time = 0;
    FILE *file = NULL;

    /* Create spaces for storing the string of the current line being read */
    char config_setting[CONFIG_BUFFER_SIZE];
    char *config_message = NULL;

    retry_time = FILE_OPEN_RETRY;
    while(retry_time--){
        file = fopen(file_name, "r");

        if(NULL != file){
            break;
        }
    }

    if (NULL == file) {
        zlog_error(category_health_report,
                   "Error openning file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning file");
#endif
        return E_OPEN_FILE;
    }

    /* Keep reading each line and store into the config struct */
    /* item 1 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_dongle_id = atoi(config_message);

    /* item 2 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_interval_in_units_0625_ms = atoi(config_message);
    
    /* item 3 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_rssi_value = atoi(config_message);

    fclose(file);

    return WORK_SUCCESSFULLY;
}

unsigned int *uuid_str_to_data(char *uuid) {
    char conversion[] = "0123456789ABCDEF";
    int uuid_length = strlen(uuid);
    unsigned int *data =
        (unsigned int *)malloc(sizeof(unsigned int) * uuid_length);

    if (data == NULL) {
        /* Error handling */
        perror("Failed to allocate memory");
        return NULL;
    }

    unsigned int *data_pointer = data;
    char *uuid_counter = uuid;

    for (; uuid_counter < uuid + uuid_length;

         data_pointer++, uuid_counter += 2) {
        *data_pointer =
            ((strchr(conversion, toupper(*uuid_counter)) - conversion) * 16) +
            (strchr(conversion, toupper(*(uuid_counter + 1))) - conversion);

    }

    return data;
}

void trim_string_tail(char *message) {

    int idx = 0;

    /* discard the whitespace, newline, carry-return characters at the end */
    if(strlen(message) > 0){

        idx = strlen(message) - 1;
        while(10 == message[idx] ||
                13 == message[idx] ||
                32 == message[idx]){

           message[idx] = '\0';
           idx--;
        }
    }
}


void ctrlc_handler(int stop) { ready_to_work = false; }

ErrorCode enable_advertising(int dongle_device_id,
                             int advertising_interval_in_units_0625_ms,
                             char *advertising_uuid,
                             int major_number,
                             int minor_number,
                             int rssi_value) {
#ifdef Debugging
    zlog_debug(category_debug, ">> enable_advertising ");
#endif
    int device_handle = 0;
    int retry_time = 0;
    uint8_t status;
    struct hci_request request;
    int return_value = 0;
    uint8_t segment_length = 1;
    unsigned int *xy_coordinates = NULL;
    int uuid_iterator;
    char uuid_identifier[17];
    int index = 0;
    int i;

#ifdef Debugging
    zlog_info(category_debug, "Using dongle id [%d] uuid [%s]\n", 
              dongle_device_id, advertising_uuid);
#endif
    //dongle_device_id = hci_get_route(NULL);
    if (dongle_device_id < 0){
        zlog_error(category_health_report,
                   "Error openning the device");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning the device");
#endif
        return E_OPEN_DEVICE;
    }

    retry_time = SOCKET_OPEN_RETRY;
    while(retry_time--){
        device_handle = hci_open_dev(dongle_device_id);

        if(device_handle >= 0){
            break;
        }
    }

    if (device_handle < 0) {
        zlog_error(category_health_report,
                   "Error openning socket");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning socket");
#endif
        return E_OPEN_DEVICE;
    }

    le_set_advertising_parameters_cp advertising_parameters_copy;
    memset(&advertising_parameters_copy, 0,
           sizeof(advertising_parameters_copy));
    advertising_parameters_copy.min_interval = 
        advertising_interval_in_units_0625_ms;
    advertising_parameters_copy.max_interval = 
        advertising_interval_in_units_0625_ms;
    /* advertising non-connectable */
    advertising_parameters_copy.advtype = 3;
    /*set bitmap to 111 (i.e., circulate on channels 37,38,39) */
    advertising_parameters_copy.chan_map = 7; /* all three advertising
                                              channels*/

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    request.cparam = &advertising_parameters_copy;
    request.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    if (return_value < 0) {
        /* Error handling */
        hci_close_dev(device_handle);
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    le_set_advertise_enable_cp advertisement_copy;
    memset(&advertisement_copy, 0, sizeof(advertisement_copy));
    advertisement_copy.enable = 0x01;

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    request.cparam = &advertisement_copy;
    request.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    if (return_value < 0) {
        /* Error handling */
        hci_close_dev(device_handle);
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    le_set_advertising_data_cp advertisement_data_copy;
    memset(&advertisement_data_copy, 0, sizeof(advertisement_data_copy));

    /* The Advertising data consists of one or more Advertising Data (AD)
    elements. Each element is formatted as follows:

    1st byte: length of the element (excluding the length byte itself)
    2nd byte: AD type – specifies what data is included in the element
    AD data - one or more bytes - the meaning is defined by AD type
    */

    /* 1. Fill the EIR_FLAGS type (0x01 in Bluetooth AD type)
    related information
    */
    segment_length = 1;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(EIR_FLAGS);
    segment_length++;

    /* FLAG information is carried in bits within the flag are as listed below,
    and we choose to use
    0x1A (i.e., 00011010) setting.
    bit 0: LE Limited Discoverable Mode
    bit 1: LE General Discoverable Mode
    bit 2: BR/EDR Not Supported
    bit 3: Simultaneous LE and BR/EDR to Same Device Capable (Controller)
    bit 4: Simultaneous LE and BR/EDR to Same Device Capable (Host)
    bit 5-7: Reserved
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x04);
    segment_length++;

    /* Fill the length for EIR_FLAGS type (0x01 in Bluetooth AD type) */
    advertisement_data_copy
        .data[advertisement_data_copy.length] =
        htobs(segment_length - 1);

    advertisement_data_copy.length += segment_length;

    /* 2. Fill the EIR_MANUFACTURE_SPECIFIC_DATA (0xFF in Bluetooth AD type)
    related information
    */
    segment_length = 1;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(EIR_MANUFACTURE_SPECIFIC_DATA);
    segment_length++;

    /* The first two bytes of EIR_MANUFACTURE_SPECIFIC_DATA type is the company
    identifier
    https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers

    For Raspberry Pi, we should use 0x000F to specify the manufacturer as
    Broadcom Corporation.
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x0F);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x00);
    segment_length++;

    /* 8 bytes: LBeacon UUID identifier.
    4 bytes for X coordinate and 4 bytes for Y coordinate.
    */
    memset(uuid_identifier, 0, sizeof(uuid_identifier));
    for(i  = 12 ; i < 20 ; i++){
        uuid_identifier[index] = *(advertising_uuid+i);
        index++;
    }
    for(i = 24 ; i < 32 ; i++){
        uuid_identifier[index] = *(advertising_uuid+i);
        index++;
    }
    xy_coordinates = uuid_str_to_data(uuid_identifier);

    for (uuid_iterator = 0;
         uuid_iterator < strlen(uuid_identifier) / 2;
         uuid_iterator++) {

        advertisement_data_copy
            .data[advertisement_data_copy.length + segment_length] =
            htobs(xy_coordinates[uuid_iterator]);

        segment_length++;
    }

    /* 1 bytes: Push-button information */
    int is_button_pressed = 0;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(is_button_pressed & 0x00FF);
    segment_length++;

    /* Fill the length for EIR_MANUFACTURE_SPECIFIC_DATA type
    (0xFF in Bluetooth AD type) */
    advertisement_data_copy.data[advertisement_data_copy.length] =
        htobs(segment_length - 1);

    advertisement_data_copy.length += segment_length;

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_DATA;
    request.cparam = &advertisement_data_copy;
    request.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    hci_close_dev(device_handle);

    if (return_value < 0) {
        /* Error handling */
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    if (status) {
        /* Error handling */
        zlog_error(category_health_report,
                   "LE set advertise returned status %d", status);
#ifdef Debugging
        zlog_error(category_debug,
                   "LE set advertise returned status %d", status);
#endif
        return E_ADVERTISE_STATUS;
    }
#ifdef Debugging
    zlog_debug(category_debug, "<< enable_advertising ");
#endif
    return WORK_SUCCESSFULLY;
}


ErrorCode disable_advertising(int dongle_device_id) {
    int device_handle = 0;
    int retry_time = 0;
    uint8_t status;
    struct hci_request request;
    int return_value = 0;
    le_set_advertise_enable_cp advertisement_copy;

#ifdef Debugging
    zlog_debug(category_debug,
               ">> disable_advertising ");
#endif
    /* Open Bluetooth device */
    retry_time = DONGLE_GET_RETRY;
    //dongle_device_id = hci_get_route(NULL);
    if (dongle_device_id < 0) {
        zlog_error(category_health_report,
                   "Error openning the device");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning the device");
#endif
        return E_OPEN_DEVICE;
    }

    retry_time = SOCKET_OPEN_RETRY;
    while(retry_time--){
        device_handle = hci_open_dev(dongle_device_id);

        if(device_handle >= 0){
            break;
        }
    }

    if (device_handle < 0) {
        zlog_error(category_health_report,
                   "Error openning socket");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning socket");
#endif
        return E_OPEN_DEVICE;
    }

    memset(&advertisement_copy, 0, sizeof(advertisement_copy));

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    request.cparam = &advertisement_copy;
    request.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    hci_close_dev(device_handle);

    if (return_value < 0) {
        /* Error handling */
        zlog_error(category_health_report,
                   "Can't set advertise mode: %s (%d)",
                   strerror(errno), errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't set advertise mode: %s (%d)",
                   strerror(errno), errno);
#endif
        return E_ADVERTISE_MODE;
    }

    if (status) {
        /* Error handling */
        zlog_error(category_health_report,
                   "LE set advertise enable on returned status %d",
                   status);
#ifdef Debugging
        zlog_error(category_debug,
                   "LE set advertise enable on returned status %d",
                   status);
#endif
        return E_ADVERTISE_STATUS;
    }
#ifdef Debugging
    zlog_debug(category_debug,
               "<< disable_advertising ");
#endif

    return WORK_SUCCESSFULLY;
}

int main(int argc, char **argv) {
    ErrorCode return_value = WORK_SUCCESSFULLY;
    struct sigaction sigint_handler;

    /*Initialize the global flag */
    ready_to_work = true;
    
    /* Initialize the application log */
    if (zlog_init("../config/zlog.conf") == 0) {

        category_health_report =
            zlog_get_category(LOG_CATEGORY_HEALTH_REPORT);

        if (!category_health_report) {
            zlog_fini();
        }

#ifdef Debugging
    	 category_debug =
           zlog_get_category(LOG_CATEGORY_DEBUG);

       if (!category_debug) {
           zlog_fini();
       }
#endif
    }

    /* Ensure there is only single running instance */
    return_value = single_running_instance(TAG_LOCK_FILE);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error openning lock file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning lock file");
#endif
        return E_OPEN_FILE;
    }

    zlog_info(category_health_report,
              "Tag process is launched...");
#ifdef Debugging
    zlog_info(category_debug,
              "Tag process is launched...");
#endif

    /* Load config struct */
    return_value = get_config(&g_config, CONFIG_FILE_NAME);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error openning config file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning config file");
#endif
        return E_OPEN_FILE;
    }

    /* Register handler function for SIGINT signal */
    sigint_handler.sa_handler = ctrlc_handler;
    sigemptyset(&sigint_handler.sa_mask);
    sigint_handler.sa_flags = 0;

    if (-1 == sigaction(SIGINT, &sigint_handler, NULL)) {
        zlog_error(category_health_report,
                   "Error registering signal handler for SIGINT");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error registering signal handler for SIGINT");
#endif
    }

    memset(lbeacon_uuid, 0, sizeof(lbeacon_uuid));
    strcpy(lbeacon_uuid, "00000000000000000000000000000000");
    
    return_value = enable_advertising(
        g_config.advertise_dongle_id,
        g_config.advertise_interval_in_units_0625_ms,
        lbeacon_uuid,
        MAJOR_VER,
        MINOR_VER,
        g_config.advertise_rssi_value);
            
    while(true == ready_to_work){
        usleep(INTERVAL_FOR_BUSY_WAITING_CHECK_IN_MICRO_SECONDS);
    }
    disable_advertising(g_config.advertise_dongle_id);

    return WORK_SUCCESSFULLY;
}
