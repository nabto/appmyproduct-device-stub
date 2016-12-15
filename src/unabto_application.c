/**
 *  uNabto application logic implementation
 */

#include "unabto/unabto_app.h"
#include <stdio.h>
#include <modules/fingerprint_acl/fp_acl_ae.h>
#include <modules/fingerprint_acl/fp_acl_memory.h>

typedef enum { HPM_COOL = 0,
               HPM_HEAT = 1,
               HPM_CIRCULATE = 2,
               HPM_DEHUMIDIFY = 3} heatpump_mode_t;

static uint8_t heatpump_state_ = 1;
static int32_t heatpump_room_temperature_ = 19;
static int32_t heatpump_target_temperature_ = 23;
static uint32_t heatpump_mode_ = HPM_HEAT;

static const char* device_name_ = "Living room";
static const char* device_product_ = "ACME 9002 Heatpump";
static const char* device_icon_ = "img/chip-small.png";

static uint8_t remote_access_enabled_ = 1;
static uint8_t open_for_pairing_ = 1;
static uint32_t default_permissions_after_pairing_ = 0;

static uint8_t user_[128];
static uint8_t user_paired_ = 0;
static uint8_t user_fingerprint_[16];
static uint8_t user_is_owner_ = 1;
static uint32_t user_permissions_ = 0;

static struct fp_acl_db db_;

void demo_init() {
    struct fp_acl_settings default_settings;
    default_settings.systemPermissions = FP_ACL_SYSTEM_PERMISSION_ALL;
    default_settings.defaultPermissions = FP_ACL_PERMISSION_ALL;
    fp_mem_init(&db_, &default_settings, NULL);
    fp_acl_ae_init(&db_);
}

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

int write_string(unabto_query_response* write_buffer, const char* string) {
    return unabto_query_write_uint8_list(write_buffer, (uint8_t *)string, strlen(string));
}

int copy_buffer(unabto_query_response* read_buffer, uint8_t* dest, uint16_t bufSize, uint16_t* len) {
    uint8_t* buffer;
    if (!(unabto_query_read_uint8_list(read_buffer, &buffer, len))) {
        return AER_REQ_TOO_SMALL;
    }
    if (*len > bufSize) {
        return AER_REQ_TOO_LARGE;
    }
    memcpy(dest, buffer, *len);
    return AER_REQ_RESPONSE_READY;
}

int copy_string(unabto_query_response* read_buffer, uint8_t* dest, uint16_t destSize) {
    uint16_t len;
    int res = copy_buffer(read_buffer, (uint8_t*)dest, destSize-1, &len);
    if (res != AER_REQ_RESPONSE_READY) {
        return res;
    }
    dest[len] = 0;
    return AER_REQ_RESPONSE_READY;
}

int write_acl(unabto_query_response* write_buffer) {
    unabto_list_ctx list;
    unabto_query_write_list_start(write_buffer, &list);

    if (!write_string(write_buffer, "2c:4d:e2:d0:2c:42:d0:2c:4d:cc:32:00:12:a5:dd:af")) return 0;
    if (!write_string(write_buffer, "Ulrik's iPhone SE")) return 0;
    if (!unabto_query_write_uint32(write_buffer, 1)) return 0;

    if (!write_string(write_buffer, "42:d0:2c:4d:cc:32:00:12:a5:dd:af:d0:2c:4d:42:d0")) return 0;
    if (!write_string(write_buffer, "Sofus' iPhone 5")) return 0;
    if (!unabto_query_write_uint32(write_buffer, 0)) return 0;

    if (!write_string(write_buffer, "87:e3:4c:57:f4:68:6c:bb:a5:dd:af:d0:2c:4d:42:d0")) return 0;
    if (!write_string(write_buffer, "Ulriks' iPad")) return 0;
    if (!unabto_query_write_uint32(write_buffer, 0)) return 0;

    if (!unabto_query_write_list_end(write_buffer, &list, 3)) return 0;

    return 1;
}

bool allow_client_access(nabto_connect* connection) {
    bool allow = fp_acl_is_connection_allowed(connection);
    NABTO_LOG_INFO(("Allowing connect request: %s", (allow ? "yes" : "no")));
    // return allow;
    
#pragma message("Always allowing client access due to AMP-87")
    return true; // local connects just time out in the simulator instead of showing access denied
}

application_event_result application_event(application_request* request,
                                           unabto_query_request* query_request,
                                           unabto_query_response* query_response) {

    NABTO_LOG_INFO(("Nabto application_event: %u", request->queryId));
    memset(user_fingerprint_, 0xff, 16);

    // handle requests as defined in interface definition shared with
    // client - for the default demo, see
    // https://github.com/nabto/ionic-starter-nabto/blob/master/www/nabto/unabto_queries.xml
    
    switch (request->queryId) {
    case 10000:
        // get_public_device_info.json
        if (!write_string(query_response, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, user_paired_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, fp_acl_is_pair_allowed(request))) return AER_REQ_RSP_TOO_LARGE;

        return AER_REQ_RESPONSE_READY;

    case 10010:
        // set_device_info.json
//        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_ADMIN)) return AER_REQ_NO_ACCESS;
        if (!write_string(query_response, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 11000:
        // get_users.json
//        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_ADMIN)) return AER_REQ_NO_ACCESS;
        return fp_acl_ae_users_get(request, query_request, query_response);
        
    case 11010: 
        // pair_with_device.json
//        if (!fp_acl_is_pair_allowed(request)) return AER_REQ_NO_ACCESS;
        user_paired_ = 1; // todo
        return fp_acl_ae_pair_with_device(request, query_request, query_response);

    case 11020:
        // get_system_security_settings.json
        return fp_acl_ae_system_get_acl_settings(request, query_request, query_response);

    case 11030:
        // get_user_permissions.json
        return fp_acl_ae_user_me(request, query_request, query_response);

    case 20000: 
        // heatpump_get_full_state.json
//        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_NONE)) return AER_REQ_NO_ACCESS; // nop
        if (!unabto_query_write_uint8(query_response, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, (uint32_t)heatpump_room_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20010:
        // heatpump_set_activation_state.json
        if (!unabto_query_read_uint8(query_request, &heatpump_state_)) return AER_REQ_TOO_SMALL;
        if (!unabto_query_write_uint8(query_response, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        NABTO_LOG_INFO(("Got (and returned) state %d", heatpump_state_));
        return AER_REQ_RESPONSE_READY;

    case 20020:
        // heatpump_set_target_temperature.json
        if (!unabto_query_read_uint32(query_request, (uint32_t*)(&heatpump_target_temperature_))) return AER_REQ_TOO_SMALL;
        if (!unabto_query_write_uint32(query_response, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20030:
        // heatpump_set_mode.json
        if (!unabto_query_read_uint32(query_request, &heatpump_mode_)) return AER_REQ_TOO_SMALL;
        if (!unabto_query_write_uint32(query_response, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    default:
        NABTO_LOG_WARN(("Unhandled query id: %u", request->queryId));
        return AER_REQ_INV_QUERY_ID;
    }
}
