cmake_minimum_required(VERSION 3.15)
project(mbedtls-custom)

# Define sources for mbedtls-custom
set(MBEDTLS_ROOT "${CMAKE_SOURCE_DIR}/mbedtls")

set(MBEDTLS_SOURCES
    ${MBEDTLS_ROOT}/library/aes.c
    ${MBEDTLS_ROOT}/library/cipher.c
    ${MBEDTLS_ROOT}/library/platform.c
    ${MBEDTLS_ROOT}/library/entropy.c
    ${MBEDTLS_ROOT}/library/ctr_drbg.c
    ${MBEDTLS_ROOT}/library/error.c
    ${MBEDTLS_ROOT}/library/md.c
    ${MBEDTLS_ROOT}/library/rsa.c
    ${MBEDTLS_ROOT}/library/sha256.c
    ${MBEDTLS_ROOT}/library/pem.c
    ${MBEDTLS_ROOT}/library/x509.c
    ${MBEDTLS_ROOT}/library/x509_crt.c
    ${MBEDTLS_ROOT}/library/x509_crl.c
    ${MBEDTLS_ROOT}/library/aesni.c
    ${MBEDTLS_ROOT}/library/platform_util.c
    ${MBEDTLS_ROOT}/library/ecdsa.c
    ${MBEDTLS_ROOT}/library/ecp.c

    # Add any other necessary source files here
)

# Add the library target
add_library(mbedtls-custom STATIC ${MBEDTLS_SOURCES})

# Include directories for mbedtls-custom
target_include_directories(mbedtls-custom PUBLIC
    ${MBEDTLS_ROOT}/include
)

# Set the output directory for the mbedtls library
set_target_properties(mbedtls-custom PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
