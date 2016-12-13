/**
 *  uNabto application logic implementation
 */

#include "unabto/unabto_app.h"
#include <unabto/unabto_util.h>
#include <modules/fingerprint_acl/fp_acl_ae.h>
#include <modules/fingerprint_acl/fp_acl_memory.h>
#include <stdio.h>

typedef enum { HPM_COOL = 0,
               HPM_HEAT = 1,
               HPM_CIRCULATE = 2,
               HPM_DEHUMIDIFY = 3} heatpump_mode_t;

static uint8_t heatpump_state_ = 0;
static int32_t heatpump_room_temperature_ = 19;
static int32_t heatpump_target_temperature_ = 23;
static uint32_t heatpump_mode_ = HPM_HEAT;

static const char* device_name_ = "Apollovej Stuen 4";
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
    fp_mem_init(&db_, NULL);
    fp_acl_ae_init(&db_);
    struct fp_acl_settings settings;
    settings.systemPermissions = FP_ACL_SYSTEM_PERMISSION_ALL;
    settings.defaultPermissions = FP_ACL_PERMISSION_ALL;
    db_.save_settings(&settings);
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

int write_string(unabto_query_response* query_response, const char* string) {
    return unabto_query_write_uint8_list(query_response, (uint8_t *)string, strlen(string));
}

int copy_buffer(unabto_query_response* query_request, uint8_t* dest, uint16_t bufSize, uint16_t* len) {
    uint8_t* buffer;
    if (!(unabto_query_read_uint8_list(query_request, &buffer, len))) {
        return AER_REQ_TOO_SMALL;
    }
    if (*len > bufSize) {
        return AER_REQ_TOO_LARGE;
    }
    memcpy(dest, buffer, *len);
    return AER_REQ_RESPONSE_READY;
}

int copy_string(unabto_query_response* query_request, uint8_t* dest, uint16_t destSize) {
    uint16_t len;
    int res = copy_buffer(query_request, (uint8_t*)dest, destSize-1, &len);
    if (res != AER_REQ_RESPONSE_READY) {
        return res;
    }
    dest[len] = 0;
    return AER_REQ_RESPONSE_READY;
}

int stub_write_acl(unabto_query_response* query_response) {
    unabto_list_ctx list;
    unabto_query_write_list_start(query_response, &list);

    if (!write_string(query_response, "Ulrik's iPhone SE")) return 0;
    if (!write_string(query_response, "2c:4d:e2:d0:2c:42:d0:2c:4d:cc:32:00:12:a5:dd:af")) return 0;
    if (!unabto_query_write_uint32(query_response, 1)) return 0;

    if (!write_string(query_response, "Sofus' iPhone 5")) return 0;
    if (!write_string(query_response, "42:d0:2c:4d:cc:32:00:12:a5:dd:af:d0:2c:4d:42:d0")) return 0;
    if (!unabto_query_write_uint32(query_response, 0)) return 0;

    if (!write_string(query_response, "Ulriks' iPad")) return 0;
    if (!write_string(query_response, "87:e3:4c:57:f4:68:6c:bb:a5:dd:af:d0:2c:4d:42:d0")) return 0;
    if (!unabto_query_write_uint32(query_response, 0)) return 0;

    if (!unabto_query_write_list_end(query_response, &list, 3)) return 0;

    return 1;
}


application_event_result application_event(application_request* request,
                                           unabto_query_request* query_request,
                                           unabto_query_response* query_response) {

    NABTO_LOG_INFO(("Nabto application_event: %u", request->queryId));

    // handle requests as defined in interface definition shared with
    // client - for the default demo, see
    // https://github.com/nabto/ionic-starter-nabto/blob/master/www/nabto/unabto_queries.xml
    
    switch (request->queryId) {
    case 10000:
        // get_public_device_info.json
        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_NONE)) return AER_REQ_NO_ACCESS; // nop
        if (!write_string(query_response, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, user_paired_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, open_for_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 10010:
        // set_device_info.json
        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_ACCESS_CONTROL)) return AER_REQ_NO_ACCESS;
        if (!write_string(query_response, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 11000:
        // get_users.json
        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_ACCESS_CONTROL)) return AER_REQ_NO_ACCESS;
        return fp_acl_ae_users_get(request, query_request, query_response);
        
    case 11010: {
        // pair_with_device.json
        if (!fp_acl_is_pair_allowed(request)) return AER_REQ_NO_ACCESS;
        int res = copy_string(query_request, user_, sizeof(user_));
        if (res != AER_REQ_RESPONSE_READY) return res;
        if (!write_string(query_response, (char*)user_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8_list(query_response, user_fingerprint_, sizeof(user_fingerprint_))) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, user_permissions_)) return AER_REQ_RSP_TOO_LARGE;
        user_paired_ = 1;
        return AER_REQ_RESPONSE_READY;
    }

    case 11020:
        // get_security_settings.json
        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_ACCESS_CONTROL)) return AER_REQ_NO_ACCESS;
        if (!unabto_query_write_uint8(query_response, user_is_owner_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, remote_access_enabled_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, open_for_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, default_permissions_after_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 11030:
        // set_security_settings.json
        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_ACCESS_CONTROL)) return AER_REQ_NO_ACCESS;
        if (!unabto_query_read_uint8(query_request, &remote_access_enabled_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_read_uint8(query_request, &open_for_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_read_uint32(query_request, &default_permissions_after_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, remote_access_enabled_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, open_for_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, default_permissions_after_pairing_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;    
        
    case 20000: 
        // heatpump_get_full_state.json
        if (!fp_acl_is_request_allowed(request, FP_ACL_PERMISSION_NONE)) return AER_REQ_NO_ACCESS;
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
