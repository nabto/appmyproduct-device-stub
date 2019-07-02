/**
 *  uNabto application logic implementation
 */

#include "unabto/unabto_app.h"
#include "unabto/unabto_types.h"
#include <stdio.h>
#include <modules/fingerprint_acl/fp_acl_ae.h>
#include <modules/fingerprint_acl/fp_acl_memory.h>
#include <modules/fingerprint_acl/fp_acl_file.h>

typedef enum { HPM_COOL = 0,
               HPM_HEAT = 1,
               HPM_CIRCULATE = 2,
               HPM_DEHUMIDIFY = 3} heatpump_mode_t;

static uint8_t heatpump_state_ = 1;
static int32_t heatpump_room_temperature_ = 19;
static int32_t heatpump_target_temperature_ = 23;
static uint32_t heatpump_mode_ = HPM_HEAT;

#define DEVICE_NAME_DEFAULT "AMP stub"
#define MAX_DEVICE_NAME_LENGTH 50
static char device_name_[MAX_DEVICE_NAME_LENGTH];
static const char* device_product_ = "ACME 9002 Heatpump";
static const char* device_icon_ = "chip-small.png";
static const char* device_interface_id_ = "317aadf2-3137-474b-8ddb-fea437c424f4";
static uint16_t device_interface_version_major_ = 1;
static uint16_t device_interface_version_minor_ = 0;

static struct fp_acl_db db_;
struct fp_mem_persistence fp_file_;

uint8_t demo_local_psk_id_[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t demo_local_psk_value_[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

#define REQUIRES_GUEST FP_ACL_PERMISSION_NONE
#define REQUIRES_OWNER FP_ACL_PERMISSION_ADMIN

#define LED0_PATH "/sys/class/leds/led0/"

void writeFile(const char* file, const char* value) {
    FILE* fout = fopen(file, "w");
    if (fout) {
        fprintf(fout, "%s", value);
        fclose(fout);
    } else {
        NABTO_LOG_INFO(("Attempt to change Raspberry Pi LED status to %s failed - are you on an RPI and do you have write permission to %s?", value, LED0_PATH));
    }
}

void updateLed() {
#if !defined(WIN32) && !defined(__MACH__) // TODO ... use some RPI specific guard
    // Blink rpi LED0 to reflect target temperature and heat pump state
    if(heatpump_state_) {
        unsigned int delay_off = 100 + (30 - heatpump_target_temperature_) * 50;
        char delay_off_str[5];
        sprintf(delay_off_str, "%u", delay_off);
        writeFile(LED0_PATH "trigger", "timer");
        writeFile(LED0_PATH "delay_on", "100");
        writeFile(LED0_PATH "delay_off", delay_off_str);
    } else {
        writeFile(LED0_PATH "brightness", "0");
    }
#endif
}

void debug_dump_acl() {
    void* it = db_.first();
    if (!it) {
        NABTO_LOG_INFO(("ACL is empty (no paired users)"));
    } else {
        NABTO_LOG_INFO(("ACL entries:"));
        while (it != NULL) {
            struct fp_acl_user user;
            fp_acl_db_status res = db_.load(it, &user);
            if (res != FP_ACL_DB_OK) {
                NABTO_LOG_WARN(("ACL error %d\n", res));
                return;
            }
            if (user.fp.hasValue) {
                NABTO_LOG_INFO((" - %s [%02x:%02x:%02x:%02x:...]: %04x",
                                user.name,
                                user.fp.value.data[0], user.fp.value.data[1], user.fp.value.data[2], user.fp.value.data[3],
                                user.permissions));
            }
            it = db_.next(it);
        }
    }
}

void demo_init() {
    struct fp_acl_settings default_settings;
    NABTO_LOG_WARN(("WARNING: Remote access to the device is turned on by default. Please read TEN36 \"Security in Nabto Solutions\" to understand the security implications."));
    default_settings.systemPermissions =
        FP_ACL_SYSTEM_PERMISSION_PAIRING |
        FP_ACL_SYSTEM_PERMISSION_LOCAL_ACCESS |
        FP_ACL_SYSTEM_PERMISSION_REMOTE_ACCESS;
    default_settings.defaultUserPermissions =
        FP_ACL_PERMISSION_LOCAL_ACCESS;
    default_settings.firstUserPermissions =
        FP_ACL_PERMISSION_ADMIN |
        FP_ACL_PERMISSION_LOCAL_ACCESS |
        FP_ACL_PERMISSION_REMOTE_ACCESS;

    if (fp_acl_file_init("persistence.bin", "tmp.bin", &fp_file_) != FP_ACL_DB_OK) {
        NABTO_LOG_ERROR(("cannot load acl file"));
        exit(1);
    }
    fp_mem_init(&db_, &default_settings, &fp_file_);
    fp_acl_ae_init(&db_);
    snprintf(device_name_, sizeof(device_name_), DEVICE_NAME_DEFAULT);
    updateLed();
    debug_dump_acl();
}

void demo_application_set_device_name(char* name) {
    strncpy(device_name_, name, MAX_DEVICE_NAME_LENGTH);
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
    static size_t ticks_ = 0;
    if ((ticks_ % 200) == 0) {
        if (heatpump_room_temperature_ < heatpump_target_temperature_) {
	    heatpump_room_temperature_++;
        } else if (heatpump_room_temperature_ > heatpump_target_temperature_) {
	    heatpump_room_temperature_--;
        }
    }
    ticks_++;
#endif
}

int copy_buffer(unabto_query_request* read_buffer, uint8_t* dest, uint16_t bufSize, uint16_t* len) {
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

int copy_string(unabto_query_request* read_buffer, char* dest, uint16_t destSize) {
    uint16_t len;
    int res = copy_buffer(read_buffer, (uint8_t*)dest, destSize-1, &len);
    if (res != AER_REQ_RESPONSE_READY) {
        return res;
    }
    dest[len] = 0;
    return AER_REQ_RESPONSE_READY;
}

int write_string(unabto_query_response* write_buffer, const char* string) {
    return unabto_query_write_uint8_list(write_buffer, (uint8_t *)string, strlen(string));
}

bool allow_client_access(nabto_connect* connection) {
    bool local = connection->isLocal;
    bool allow = fp_acl_is_connection_allowed(connection) || local;
    NABTO_LOG_INFO(("Allowing %s connect request: %s", (local ? "local" : "remote"), (allow ? "yes" : "no")));
    debug_dump_acl();
    return allow;
}

application_event_result application_event(application_request* request,
                                           unabto_query_request* query_request,
                                           unabto_query_response* query_response) {

    NABTO_LOG_INFO(("Nabto application_event: %u", request->queryId));
    debug_dump_acl();

    // handle requests as defined in interface definition shared with
    // client - for the default demo, see
    // https://github.com/nabto/ionic-starter-nabto/blob/master/www/nabto/unabto_queries.xml

    application_event_result res;

    if (request->queryId >= 11000 && request->queryId < 12000) {
        // default PPKA access control (see unabto/src/modules/fingerprint_acl/fp_acl_ae.c)
        application_event_result res = fp_acl_ae_dispatch(11000, request, query_request, query_response);
        NABTO_LOG_INFO(("ACL request [%d] handled with status %d", request->queryId, res));
        debug_dump_acl();
        return res;
    }

    switch (request->queryId) {
    case 0:
        // get_interface_info.json
        if (!write_string(query_response, device_interface_id_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint16(query_response, device_interface_version_major_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint16(query_response, device_interface_version_minor_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 10000:
        // get_public_device_info.json
        if (!write_string(query_response, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_product_)) return AER_REQ_RSP_TOO_LARGE;
        if (!write_string(query_response, device_icon_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, fp_acl_is_pair_allowed(request))) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, fp_acl_is_user_paired(request))) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint8(query_response, fp_acl_is_user_owner(request))) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 10010:
        // set_device_info.json
        if (!fp_acl_is_request_allowed(request, REQUIRES_OWNER)) return AER_REQ_NO_ACCESS;
        int res = copy_string(query_request, device_name_, sizeof(device_name_));
        if (res != AER_REQ_RESPONSE_READY) return res;
        if (!write_string(query_response, device_name_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20000:
        // heatpump_get_full_state.json
        if (!fp_acl_is_request_allowed(request, REQUIRES_GUEST)) return AER_REQ_NO_ACCESS;
        if (!unabto_query_write_uint8(query_response, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        if (!unabto_query_write_uint32(query_response, (uint32_t)heatpump_room_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    case 20010:
        // heatpump_set_activation_state.json
        if (!fp_acl_is_request_allowed(request, REQUIRES_GUEST)) return AER_REQ_NO_ACCESS;
        if (!unabto_query_read_uint8(query_request, &heatpump_state_)) return AER_REQ_TOO_SMALL;
        if (!unabto_query_write_uint8(query_response, heatpump_state_)) return AER_REQ_RSP_TOO_LARGE;
        NABTO_LOG_INFO(("Got (and returned) state %d", heatpump_state_));
        updateLed();
        return AER_REQ_RESPONSE_READY;

    case 20020:
        // heatpump_set_target_temperature.json
        if (!fp_acl_is_request_allowed(request, REQUIRES_GUEST)) return AER_REQ_NO_ACCESS;
        if (!unabto_query_read_uint32(query_request, (uint32_t*)(&heatpump_target_temperature_))) return AER_REQ_TOO_SMALL;
        if (!unabto_query_write_uint32(query_response, (uint32_t)heatpump_target_temperature_)) return AER_REQ_RSP_TOO_LARGE;
        updateLed();
        return AER_REQ_RESPONSE_READY;

    case 20030:
        // heatpump_set_mode.json
        if (!fp_acl_is_request_allowed(request, REQUIRES_GUEST)) return AER_REQ_NO_ACCESS;
        if (!unabto_query_read_uint32(query_request, &heatpump_mode_)) return AER_REQ_TOO_SMALL;
        if (!unabto_query_write_uint32(query_response, heatpump_mode_)) return AER_REQ_RSP_TOO_LARGE;
        return AER_REQ_RESPONSE_READY;

    default:
        NABTO_LOG_WARN(("Unhandled query id: %u", request->queryId));
        return AER_REQ_INV_QUERY_ID;
    }
}

#if NABTO_ENABLE_LOCAL_PSK_CONNECTION
bool unabto_local_psk_connection_get_key(const struct unabto_psk_id* keyId, const char* clientId, const struct unabto_optional_fingerprint* pkFp, struct unabto_psk* key) {
    if (memcmp(keyId, demo_local_psk_id_, PSK_ID_LENGTH) == 0) {
        memcpy(&key->data, demo_local_psk_value_, PSK_LENGTH);
        return true;
    } else {
        NABTO_LOG_WARN(("Key [%02x:%02x:%02x:...] is not known",
                        keyId->data[0], keyId->data[1], keyId->data[2]));
        return false;
    }
}
#endif
