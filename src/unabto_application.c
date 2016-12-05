/**
 *  uNabto application logic implementation
 */

#include "unabto/unabto_app.h"
#include <stdio.h>
#include "LEDBlink.c"

typedef enum { HPM_COOL = 0,
               HPM_HEAT = 1,
               HPM_CIRCULATE = 2,
               HPM_DEHUMIDIFY = 3} heatpump_mode_t;

static uint8_t heatpump_state_ = 1;
static int32_t heatpump_room_temperature_ = 19;
static int32_t heatpump_target_temperature_ = 23;
static uint32_t heatpump_mode_ = HPM_HEAT;

static const char* device_name_ = "Apollovej Stuen 4";
static const char* device_product_ = "ACME 9002 Heatpump";
static const char* device_icon_ = "img/chip-small.png";

void demo_application_set_device_name(const char* name) {
    device_name_ = name;
}

void demo_application_set_device_product(const char* product) {
    device_product_ = product;
}

void demo_application_set_device_icon_(const char* icon) {
    device_icon_ = icon;
}

void demo_application_tick() {
#ifndef WIN32
    static time_t time_last_update_ = 0;
    time_t now = time(0);
    if (now - time_last_update_ > 2) {
        if (heatpump_room_temperature_ < heatpump_target_temperature_) {
            heatpump_room_temperature_++;
        } else if (heatpump_room_temperature_ > heatpump_target_temperature_) {
            heatpump_room_temperature_--;
        }
        time_last_update_ = now;
    }
#else
    size_t ticks_ = 0;
    heatpump_room_temperature_ = heatpump_target_temperature_ + ticks++ % 2;
#endif
}

int write_string(buffer_write_t* write_buffer, const char* string) {
    return unabto_query_write_uint8_list(write_buffer, (uint8_t *)string, strlen(string));
}

int write_acl(buffer_write_t* write_buffer) {
    unabto_list_ctx list;
    unabto_query_write_list_start(write_buffer, &list);

    if (!write_string(write_buffer, "Ulrik's iPhone SE")) return 0;
    if (!write_string(write_buffer, "2c:4d:e2:d0:2c:42:d0:2c:4d:cc:32:00:12:a5:dd:af")) return 0;
    if (!unabto_query_write_uint32(write_buffer, 1)) return 0;

    if (!write_string(write_buffer, "Sofus' iPhone 5")) return 0;
    if (!write_string(write_buffer, "42:d0:2c:4d:cc:32:00:12:a5:dd:af:d0:2c:4d:42:d0")) return 0;
    if (!unabto_query_write_uint32(write_buffer, 0)) return 0;

    if (!write_string(write_buffer, "Ulriks' iPad")) return 0;
    if (!write_string(write_buffer, "87:e3:4c:57:f4:68:6c:bb:a5:dd:af:d0:2c:4d:42:d0")) return 0;
    if (!unabto_query_write_uint32(write_buffer, 0)) return 0;

    if (!unabto_query_write_list_end(write_buffer, &list, 3)) return 0;

    return 1;
}

application_event_result application_event(application_request* request,
                                           buffer_read_t* read_buffer,
                                           buffer_write_t* write_buffer) {

    NABTO_LOG_INFO(("Nabto application_event: %u", request->queryId));

    // handle requests as defined in interface definition shared with
    // client - for the default demo, see
    // https://github.com/nabto/ionic-starter-nabto/blob/master/www/nabto/unabto_queries.xml
    
    switch (request->queryId) {
    case 10000:
        // get_public_device_info.json
        if (!write_string(write_buffer, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(write_buffer, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(write_buffer, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 10010:
        // set_device_info.json
        if (!write_string(write_buffer, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(write_buffer, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(write_buffer, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 11000:
        // get_users.json
        if (!write_acl(write_buffer)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20000: 
        // heatpump_get_full_state.json
        if (!buffer_write_uint8(write_buffer, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        if (!buffer_write_uint32(write_buffer, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        if (!buffer_write_uint32(write_buffer, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        if (!buffer_write_uint32(write_buffer, (uint32_t)heatpump_room_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20010:
        // heatpump_set_activation_state.json
        if (!buffer_read_uint8(read_buffer, &heatpump_state_)) return AER_REQ_TOO_SMALL;
        if (!buffer_write_uint8(write_buffer, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        NABTO_LOG_INFO(("Got (and returned) state %d", heatpump_state_));
        return AER_REQ_RESPONSE_READY;

    case 20020:
        // heatpump_set_target_temperature.json
        if (!buffer_read_uint32(read_buffer, (uint32_t*)(&heatpump_target_temperature_))) return AER_REQ_TOO_SMALL;
        if (!buffer_write_uint32(write_buffer, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
	int Temperature = (int)heatpump_target_temperature_;
	LEDBlink_SetTemperature(Temperature);
        return AER_REQ_RESPONSE_READY;

    case 20030:
        // heatpump_set_mode.json
        if (!buffer_read_uint32(read_buffer, &heatpump_mode_)) return AER_REQ_TOO_SMALL;
        if (!buffer_write_uint32(write_buffer, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    default:
        NABTO_LOG_WARN(("Unhandled query id: %u", request->queryId));
        return AER_REQ_INV_QUERY_ID;
    }
}
