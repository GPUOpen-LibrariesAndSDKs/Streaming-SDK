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

#pragma once

#include <memory>
#ifdef _WIN32
#include <Wincrypt.h>
#endif

// Single thread for mbedtls, multi thread for tiny aes
#define SINGLE_THREADED_DECRYPT     1
#define MBEDTLS_AES 1

namespace ssdk::util
{

    class AESPSKCipher
    {
    public:
        enum {
            KEY_SIZE = 32,
            AES_256 = 256,
            AES_192 = 192,
            AES_128 = 128,
            BLOCK_SIZE = 16,
            IV_SIZE = BLOCK_SIZE
        };

        enum Flags
        {
            FLAGS_SCHEME_CBC = 1,
            FLAGS_SINGLE_FRAGMENT = 0
        };

    public:
        typedef std::shared_ptr<AESPSKCipher>    Ptr;

        AESPSKCipher(const char* passphrase, const void* salt = nullptr, size_t saltSize = 0);
        AESPSKCipher(const void* key);
        virtual ~AESPSKCipher();

        // ans::ANSCipher methods
        // Returns the size of cipher text for the given size of clear text, add the size of any necessary headers, padding, etc.This is called before every call to Encrypt
        size_t GetCipherTextBufferSize(size_t clearTextSize) const;

        // Encrypt a message. cipherText receives a buffer of the size returned by GetCipherTextSize
        bool Encrypt(const void* clearText, size_t clearTextSize, void* cipherText, size_t* cipherTextSize);

        // Returns the size of cleartext for the given size of ciphertext, subtract the size of any necessary headers, padding, etc.This is called before every call to Decrypt
        size_t GetClearTextBufferSize(size_t cipherTextSize) const;

        // Decrypt a message. The buffer for clear text is allocated with the size passed in cipherTextLength, but the actual clear text might be smaller due to removal of headers,
        // padding, etc and is returned as the method's return value
        bool Decrypt(const void* cipherText, size_t cipherTextSize, void* clearText, size_t* clearTextOfs, size_t* clearTextSize);

        // Key management methods:
        // Calculate the encryption key from a passphrase. When passphraseLength==0, the passphrase is assumed to be a NULL-terminated string
        bool SetPassphrase(const void* passphrase);

    private:
        void Init();

        static size_t CalculateAlignedSize(size_t orgSize);

    private:
        std::unique_ptr<uint8_t[]>  m_Salt;
        size_t      m_SaltSize;

        uint8_t     m_Key[KEY_SIZE];

#ifdef _WIN32
        HCRYPTPROV m_hCryptProv;
#endif

    };
}