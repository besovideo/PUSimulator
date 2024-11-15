#ifndef __LIB_AAA_CONFIG_H__
#define __LIB_AAA_CONFIG_H__


#define AAA_TEST_FUNCTION  0
#define MG_ENABLE_LOG      1
#define SSL_WITH_WBEDTLS



#ifdef SSL_WITH_WBEDTLS
// mbedtls
#   define MG_ENABLE_MBEDTLS  1
#else
// openssl
#   define MG_ENABLE_OPENSSL  1
#endif

#endif
