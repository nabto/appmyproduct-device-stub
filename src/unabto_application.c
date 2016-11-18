/**
 *  uNabto application logic implementation
 */

#include "unabto/unabto_app.h"
#include <stdio.h>

static uint8_t heatpump_state_ = 0;
static int32_t heatpump_room_temperature_ = 0;
static int32_t heatpump_target_temperature_ = 0;
static uint32_t heatpump_mode_ = 0;

static const char* device_name = "Apollovej Stuen 4";
static const char* device_type = "ACME 9002 Heatpump";
static const char* device_icon = "img/chip-small.png";


void demo_application_tick() {
#ifndef WIN32
    static time_t time_last_update_ = 0;
    time_t now = time(0);
    if (now - time_last_update_ > 2) {
        if (heatpump_room_temperature_ < heatpump_target_temperature_) {
            heatpump_room_temperature_++;
        } else if (heatpump_room_temperature_ < heatpump_target_temperature_) {
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

application_event_result application_event(application_request* request,
                                           buffer_read_t* read_buffer,
                                           buffer_write_t* write_buffer) {

    NABTO_LOG_INFO(("Nabto application_event: %u", request->queryId));

    // handle requests as defined in interface definition shared with
    // client - for the default demo, see
    // https://github.com/nabto/ionic-starter-nabto/blob/master/www/nabto/unabto_queries.xml
    
    switch (request->queryId) {
    case 10000:
        if (!write_string(write_buffer, device_name)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(write_buffer, device_type)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(write_buffer, device_icon)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;
        
    case 20000: 
        // heatpump_get_state.json
        if (!buffer_write_uint8(write_buffer, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;
        
    case 20010:
        // heatpump_set_state.json
        if (!buffer_read_uint8(read_buffer, &heatpump_state_)) return AER_REQ_TOO_SMALL;
        if (!buffer_write_uint8(write_buffer, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20020:
        // heatpump_get_room_temperature.json
        if (!buffer_write_uint32(write_buffer, (uint32_t)heatpump_room_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20030:
        // heatpump_get_target_temperature.json
        if (!buffer_write_uint32(write_buffer, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20040:
        // heatpump_set_target_temperature.json
        if (!buffer_read_uint32(read_buffer, (uint32_t*)(&heatpump_target_temperature_))) return AER_REQ_TOO_SMALL;
        if (!buffer_write_uint32(write_buffer, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20050:
        // heatpump_get_mode.json
        if (!buffer_write_uint32(write_buffer, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20060:
        // heatpump_get_mode.json
        if (!buffer_read_uint32(read_buffer, &heatpump_mode_)) return AER_REQ_TOO_SMALL;
        if (!buffer_write_uint32(write_buffer, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    default:
        NABTO_LOG_WARN(("Unhandled query id: %u", request->queryId));
        return AER_REQ_INV_QUERY_ID;
    }
}













    
