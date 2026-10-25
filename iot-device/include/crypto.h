#ifndef CRYPTO_H
#define CRYPTO_H

#include <Arduino.h>

String calculateHMAC(const char* payload, const char* secret);
bool verifyHMAC(const char* payload, const char* signature, const char* secret);
String generateNonce();

#endif // CRYPTO_H
