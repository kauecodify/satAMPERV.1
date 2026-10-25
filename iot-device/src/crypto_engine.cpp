#include "crypto.h"
#include <mbedtls/md.h>
#include <string.h>
#include <stdio.h>

std::string calculate_hmac(const std::string& payload, const std::string& secret) {
    unsigned char hash[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 = HMAC
    
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload.c_str(), payload.length());
    mbedtls_md_hmac_finish(&ctx, hash);
    mbedtls_md_free(&ctx);
    
    // Converte para Hex (simplificado)
    char hex_string[65];
    for (int i = 0; i < 32; i++) {
        sprintf(&hex_string[i * 2], "%02x", hash[i]);
    }
    
    return std::string(hex_string);
}
