//
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
//
// MIT license
//
//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// The implementation of this cipher follows the recommendations outlined in:
// RFC3602: https://tools.ietf.org/html/rfc3602#section-3
// RFC4347: https://tools.ietf.org/html/rfc4347#section-3.1
// RFC2406: https://tools.ietf.org/html/rfc2406#section-2.3
// and adheres to the same general approach as DTLS

#include "amf/public/include/core/Trace.h"
#include "amf/public/common/TraceAdapter.h"
#include "amf/public/common/Thread.h"
#include "AESPSKCipher.h"
#include "mbedtls/include/mbedtls/aes.h"
#define AES_BLOCKLEN 16 // Block length in bytes - AES is 128b block only, tiny aes provides this macro.
#include "mbedtls/include/mbedtls/sha256.h"
#include <string>
#include <cstring>
#ifndef _WIN32
    #include <arpa/inet.h>
#endif

static constexpr const wchar_t* const AMF_FACILITY = L"AESPSKCipher";
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace ssdk::util
{
#pragma pack(push, 1)
    class UnencryptedMessageHeader
    {
    public:
        UnencryptedMessageHeader() : m_Flags(0) { memset(m_Iv, 0, sizeof(m_Iv)); }
        UnencryptedMessageHeader(const uint8_t* iv, uint16_t flags) : m_Flags(htons(flags)) { memcpy(m_Iv, iv, sizeof(m_Iv)); }

        inline const uint8_t* GetIV() const noexcept { return m_Iv; }
        inline uint16_t Flags() const noexcept { return ntohs(m_Flags); }
    private:
        uint16_t    m_Flags;
        uint8_t     m_Iv[AESPSKCipher::IV_SIZE];
    };

    class EncryptedMessageHeader
    {
    public:
        EncryptedMessageHeader() : m_OrgSize(0) {}
        EncryptedMessageHeader(size_t size) : m_OrgSize(htonl(static_cast<uint32_t>(size))) {}

        inline uint32_t OrgSize() const noexcept { return ntohl(m_OrgSize); }

    private:
        uint32_t    m_OrgSize;
    };
#pragma pack(pop)

    AESPSKCipher::AESPSKCipher(const char* passphrase, const void* salt, size_t saltSize) :
        m_SaltSize(0)
    {
        if (salt != nullptr)
        {
            if (saltSize == 0)
            {
                m_SaltSize = std::strlen(static_cast<const char*>(salt)) + 1;
            }
            else
            {
                m_SaltSize = saltSize;
            }
            if (m_SaltSize > 0)
            {
                m_Salt = std::unique_ptr<uint8_t[]>(new uint8_t[m_SaltSize]);
                memcpy(m_Salt.get(), salt, m_SaltSize);
            }
        }
        SetPassphrase(passphrase);
        Init();
    }

    AESPSKCipher::AESPSKCipher(const void* key) :
        m_SaltSize(0)
    {
        memcpy(m_Key, key, sizeof(m_Key));
        Init();
    }

    void AESPSKCipher::Init()
    {
#ifdef _WIN32
        CryptAcquireContext(&m_hCryptProv, NULL,
            (LPCWSTR)L"Microsoft Base Cryptographic Provider v1.0",
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT);
#endif
    }

    AESPSKCipher::~AESPSKCipher()
    {
#ifdef _WIN32
        CryptReleaseContext(m_hCryptProv, 0);
#endif
    }

    size_t AESPSKCipher::CalculateAlignedSize(size_t orgSize)
    {
        size_t adjustedSize = orgSize - 1;
        return adjustedSize + (AES_BLOCKLEN - adjustedSize % AES_BLOCKLEN);
    }

    // Returns the size of cipher text for the given size of clear text, add the size of any necessary headers, padding, etc.This is called before every call to Encrypt
    size_t AMF_STD_CALL AESPSKCipher::GetCipherTextBufferSize(size_t clearTextSize) const
    {
        return CalculateAlignedSize(clearTextSize + sizeof(EncryptedMessageHeader)) + sizeof(UnencryptedMessageHeader);
    }

    // Encrypt a message. cipherText receives a buffer of the size returned by GetCipherTextSize
    bool AESPSKCipher::Encrypt(const void* clearText, size_t clearTextSize, void* cipherText, size_t* cipherTextSize)
    {
        uint8_t     iv[IV_SIZE];
#ifdef _WIN32
        CryptGenRandom(m_hCryptProv, sizeof(iv), iv);
#else   //  Populate iv with a random value produced from timer
        amf_pts     now = amf_high_precision_clock();
        *reinterpret_cast<uint64_t*>(iv) = now;
        *(reinterpret_cast<uint64_t*>(iv) + 1) = (now << 7) ^ now;
#endif

        UnencryptedMessageHeader unencrypedHeader(iv, FLAGS_SCHEME_CBC | FLAGS_SINGLE_FRAGMENT);
        EncryptedMessageHeader encryptedHeader(clearTextSize);
        memcpy(cipherText, &unencrypedHeader, sizeof(unencrypedHeader));
        memcpy(static_cast<uint8_t*>(cipherText) + sizeof(unencrypedHeader), &encryptedHeader, sizeof(encryptedHeader));
        memcpy(static_cast<uint8_t*>(cipherText) + sizeof(unencrypedHeader) + sizeof(encryptedHeader), clearText, clearTextSize);

        mbedtls_aes_context     AESContext;
        uint8_t* startOfCipher = static_cast<uint8_t*>(cipherText) + sizeof(unencrypedHeader);
        int result = 0;
        size_t encryptionLength = CalculateAlignedSize(clearTextSize + sizeof(EncryptedMessageHeader));
        mbedtls_aes_init(&AESContext);
        result = mbedtls_aes_setkey_enc(&AESContext, m_Key, AESPSKCipher::AES_128);
        if (result != 0)
        {
            AMFTraceError(AMF_FACILITY, L"failed to set encryption key, return code:%d", result);
            mbedtls_aes_free(&AESContext);
            return false;
        }
        result = mbedtls_aes_crypt_cbc(&AESContext, MBEDTLS_AES_ENCRYPT, encryptionLength, iv, startOfCipher, startOfCipher);
        if (result != 0)
        {
            AMFTraceError(AMF_FACILITY, L"failed to encrypt, return code:%d", result);
            mbedtls_aes_free(&AESContext);
            return false;
        }
        mbedtls_aes_free(&AESContext);
        * cipherTextSize = GetCipherTextBufferSize(clearTextSize);
        return true;
    }

    // Returns the size of cleartext for the given size of ciphertext, subtract the size of any necessary headers, padding, etc.This is called before every call to Decrypt
    size_t AESPSKCipher::GetClearTextBufferSize(size_t cipherTextSize) const
    {
        return cipherTextSize > sizeof(UnencryptedMessageHeader) ? CalculateAlignedSize(cipherTextSize - sizeof(UnencryptedMessageHeader)) : 0;
    }

    // Decrypt a message. The buffer for clear text is allocated with the size passed in cipherTextLength, but the actual clear text might be smaller due to removal of headers,
    // padding, etc and is returned as the method's return value


    bool AESPSKCipher::Decrypt(const void* cipherText, size_t cipherTextSize, void* clearText, size_t* clearTextOfs, size_t* clearTextSize)
    {
        AMF_RESULT result = AMF_FAIL;
        UnencryptedMessageHeader unencrypedHeader;
        memcpy(&unencrypedHeader, cipherText, sizeof(unencrypedHeader));
        if (unencrypedHeader.Flags() == (FLAGS_SCHEME_CBC | FLAGS_SINGLE_FRAGMENT))
        {
            uint32_t bytesToDecrypt = static_cast<uint32_t>(cipherTextSize) - sizeof(unencrypedHeader);
            memcpy(clearText, static_cast<const uint8_t*>(cipherText) + sizeof(unencrypedHeader), bytesToDecrypt);
            uint8_t iv[AESPSKCipher::IV_SIZE];
            memcpy(iv, unencrypedHeader.GetIV(), AESPSKCipher::IV_SIZE);
            int mbedResult = 0;

            mbedtls_aes_context     AESContext;
            mbedtls_aes_init(&AESContext);
            mbedResult = mbedtls_aes_setkey_dec(&AESContext, m_Key, AESPSKCipher::AES_128);
            if (mbedResult != 0)
            {
                AMFTraceError(AMF_FACILITY, L"failed to set decryption key, return code:%d", result);
                mbedtls_aes_free(&AESContext);
                return false;
            }
            mbedResult = mbedtls_aes_crypt_cbc(&AESContext, MBEDTLS_AES_DECRYPT, bytesToDecrypt, iv, static_cast<uint8_t*>(clearText), static_cast<uint8_t*>(clearText));
            if (mbedResult != 0) {
                AMFTraceError(AMF_FACILITY, L"failed to decrypt, return code:%d", result);
                mbedtls_aes_free(&AESContext);
                return false;
            }
            mbedtls_aes_free(&AESContext);
            * clearTextSize = static_cast<EncryptedMessageHeader*>(clearText)->OrgSize();
            *clearTextOfs = sizeof(EncryptedMessageHeader);
            result = AMF_OK;
        }
        return result == AMF_OK;
    }

    bool AESPSKCipher::SetPassphrase(const void* passphrase)
    {
        bool result = false;
        if (passphrase != nullptr)
        {
            size_t length = std::strlen(static_cast<const char*>(passphrase)) + 1;
            std::unique_ptr<uint8_t[]> saltedPassphrase;
            if (m_Salt != nullptr)
            {
                size_t saltedPassphraseLength = length + m_SaltSize;
                saltedPassphrase = std::unique_ptr<uint8_t[]>(new uint8_t[saltedPassphraseLength]);
                memcpy(saltedPassphrase.get(), m_Salt.get(), m_SaltSize);
            }
            else
            {
                saltedPassphrase = std::unique_ptr<uint8_t[]>(new uint8_t[length]);
            }
            memcpy(saltedPassphrase.get() + m_SaltSize, passphrase, length);
            result = (mbedtls_sha256(saltedPassphrase.get(), length + m_SaltSize, m_Key, 0) == 0);
        }
        return result;
    }
}