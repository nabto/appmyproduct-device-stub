#ifndef _UNABTO_CONFIG_H_
#define _UNABTO_CONFIG_H_

#include <modules/log/dynamic/unabto_dynamic_log.h>

#define NABTO_ENABLE_CONNECTION_ESTABLISHMENT_ACL_CHECK 1

#define NABTO_ENABLE_STREAM                         0
#define NABTO_CONNECTIONS_SIZE                      500
#define NABTO_ENABLE_UCRYPTO                        1
#define NABTO_ENABLE_LOCAL_PSK_CONNECTION           1
#define NABTO_ENABLE_LOGGING                        1

#ifdef LOG_ALL
#define NABTO_LOG_ALL 1
#endif
    
#endif
