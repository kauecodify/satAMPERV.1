cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# ── Metadados do projeto ─────────────────────────────────────
set(PROJECT_NAME       "iot-device")
set(PROJECT_VER        "1.2.0")
set(PROJECT_VER_MAJOR  1)
set(PROJECT_VER_MINOR  2)
set(PROJECT_VER_PATCH  0)

# ── Componentes locais ───────────────────────────────────────
# Adicionados via components/ (auto-discovery do IDF).
# Exceção: failsafe_policy também é compilado no host.

# ── Flags de compilação (segurança) ──────────────────────────
add_compile_options(
    -Wall -Wextra -Werror
    -Wshadow -Wconversion -Wundef
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
)

# ── Configuração de Flash Encryption (produção) ──────────────
# Requer fuses queimados; documentado em docs/prod-fuses.md
# add_compile_definitions(CONFIG_SECURE_FLASH_ENC_ENABLED)

project(${PROJECT_NAME})
