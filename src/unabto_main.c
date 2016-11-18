/**
 *  Implementation of main for uNabto SDK
 */

#include "unabto/unabto_env_base.h"
#include "unabto/unabto_common_main.h"
#include "unabto/unabto_logging.h"
#include <modules/util/read_hex.h>

void nabto_yield(int msec);
static void help(const char* errmsg, const char *progname);

int main(int argc, char* argv[])
{
    if (argc != 3) {
        help(0, argv[0]);
        exit(1);
    }

    nabto_main_setup* nms = unabto_init_context();
    nms->secureAttach = true;
    nms->secureData = true;
    nms->cryptoSuite = CRYPT_W_AES_CBC_HMAC_SHA256;

    nms->id = strdup(argv[1]);

    if (!unabto_read_psk_from_hex(argv[2], nms->presharedKey, 16)) {
        help("Invalid cryptographic key specified", argv[0]);
        return false;
    }

    if (!unabto_init()) {
        NABTO_LOG_FATAL(("Failed at nabto_main_init"));
    }

    NABTO_LOG_INFO(("AppMyProduct demo stub [%s] running!", nms->id));

    while (true) {
        unabto_tick();
        nabto_yield(10);
    }

    unabto_close();
    return 0;
}

void nabto_yield(int msec)
{
#ifdef WIN32
    Sleep(msec);
#elif defined(__MACH__)
    if (msec) usleep(1000*msec);
#else
    if (msec) usleep(1000*msec); else sched_yield();
#endif
}

static void help(const char* errmsg, const char *progname)
{
    if (errmsg) {
        printf("ERROR: %s\n", errmsg);
    }
    printf("\nAppMyProduct demo stub application to try the platform and example apps.\n");
    printf("Obtain a device id and crypto key from www.appmyproduct.com\n\n");
    printf("Usage: %s <device id> <crypto key>\n", progname);
}

    
