#include "crypto.h"
#include <mbedtls/sha256.h>
#include <mbedtls/md.h>

// Calcular HMAC-SHA256
String calculateHMAC(const char* payload, const char* secret) {
    unsigned char hash[32];
    unsigned char result[32];
    
    // Contexto HMAC
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    
    // Configurar SHA256
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 = HMAC
    
    // Configurar chave
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret, strlen(secret));
    
    // Processar payload
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload, strlen(payload));
    
    // Finalizar
    mbedtls_md_hmac_finish(&ctx, result);
    
    // Converter para hexadecimal
    String hexResult = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", result[i]);
        hexResult += hex;
    }
    
    mbedtls_md_free(&ctx);
    
    return hexResult;
}

// Verificar HMAC
bool verifyHMAC(const char* payload, const char* signature, const char* secret) {
    String calculated = calculateHMAC(payload, secret);
    return calculated.equals(signature);
}

// Gerar nonce aleatório
String generateNonce() {
    String nonce = "";
    for (int i = 0; i < 16; i++) {
        char hex[3];
        sprintf(hex, "%02x", random(256));
        nonce += hex;
    }
    return nonce;
}
