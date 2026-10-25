// secure_mavlink.cpp - Comunicação MAVLink com criptografia HMAC
#include <Arduino.h>
#include <MAVLink.h>
#include <mbedtls/sha256.h>
#include "config.h"

class SecureMAVLink {
private:
    HardwareSerial* serial;
    const char* hmac_secret;
    
    // Estrutura para mensagem assinada
    struct SignedMessage {
        uint8_t mavlink_payload[MAVLINK_MAX_PACKET_LEN];
        uint16_t mavlink_length;
        char signature[65];  // HMAC-SHA256 em hex (64 chars + null terminator)
    };
    
public:
    SecureMAVLink(HardwareSerial* serial_port, const char* secret) {
        serial = serial_port;
        hmac_secret = secret;
    }
    
    // Enviar mensagem MAVLink assinada
    void sendSignedMessage(mavlink_message_t& msg) {
        // Serializar mensagem MAVLink
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        uint16_t len = mavlink_msg_to_send_buffer(buffer, &msg);
        
        // Calcular HMAC da mensagem
        char signature[65];
        calculateHMAC((const char*)buffer, len, signature);
        
        // Enviar payload + assinatura (formato customizado)
        // Formato: [LENGTH:2][PAYLOAD:LEN][SIGNATURE:64]
        uint8_t length_bytes[2];
        length_bytes[0] = len & 0xFF;
        length_bytes[1] = (len >> 8) & 0xFF;
        
        serial->write(length_bytes, 2);
        serial->write(buffer, len);
        serial->print(signature);
        
        Serial.printf("Mensagem MAVLink enviada e assinada (%d bytes)\n", len);
    }
    
    // Receber e verificar mensagem MAVLink assinada
    bool receiveSignedMessage(mavlink_message_t& msg) {
        // Ler tamanho da mensagem
        if (serial->available() < 2) return false;
        
        uint8_t length_bytes[2];
        serial->readBytes(length_bytes, 2);
        uint16_t len = length_bytes[0] | (length_bytes[1] << 8);
        
        if (len > MAVLINK_MAX_PACKET_LEN) {
            Serial.println("Erro: Tamanho de mensagem inválido");
            return false;
        }
        
        // Ler payload
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        if (serial->available() < len) return false;
        serial->readBytes(buffer, len);
        
        // Ler assinatura
        char received_signature[65];
        if (serial->available() < 64) return false;
        serial->readBytes(received_signature, 64);
        received_signature[64] = '\0';
        
        // Calcular HMAC esperado
        char expected_signature[65];
        calculateHMAC((const char*)buffer, len, expected_signature);
        
        // Verificar assinatura
        if (strcmp(received_signature, expected_signature) != 0) {
            Serial.println("ERRO: Assinatura inválida! Mensagem rejeitada.");
            return false;
        }
        
        // Parsear mensagem MAVLink
        mavlink_status_t status;
        for (uint16_t i = 0; i < len; i++) {
            if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status)) {
                Serial.println("Mensagem MAVLink recebida e verificada com sucesso");
                return true;
            }
        }
        
        return false;
    }
    
private:
    // Calcular HMAC-SHA256
    void calculateHMAC(const char* data, size_t length, char* output) {
        unsigned char hash[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        
        const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        mbedtls_md_setup(&ctx, md_info, 1);
        mbedtls_md_hmac_starts(&ctx, (const unsigned char*)hmac_secret, strlen(hmac_secret));
        mbedtls_md_hmac_update(&ctx, (const unsigned char*)data, length);
        mbedtls_md_hmac_finish(&ctx, hash);
        mbedtls_md_free(&ctx);
        
        // Converter para hexadecimal
        for (int i = 0; i < 32; i++) {
            sprintf(&output[i * 2], "%02x", hash[i]);
        }
        output[64] = '\0';
    }
};

// Uso exemplo:
// HardwareSerial mavlinkSerial(2);
// SecureMAVLink secureLink(&mavlinkSerial, CONFIG_HMAC_SECRET);
// 
// mavlink_message_t msg;
// mavlink_msg_heartbeat_pack(1, 1, &msg, ...);
// secureLink.sendSignedMessage(msg);
