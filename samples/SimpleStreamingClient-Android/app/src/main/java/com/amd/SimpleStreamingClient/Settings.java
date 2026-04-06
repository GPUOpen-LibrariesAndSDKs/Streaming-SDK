/*
Notice Regarding Standards.  AMD does not provide a license or sublicense to
any Intellectual Property Rights relating to any standards, including but not
limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
(collectively, the "Media Technologies"). For clarity, you will pay any
royalties due for such third party technologies, which may include the Media
Technologies that are owed as a result of AMD providing the Software to you.

This software uses libraries from the FFmpeg project under the LGPLv2.1.

MIT license

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

package com.amd.SimpleStreamingClient;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.MasterKeys;

import java.io.IOException;
import java.security.GeneralSecurityException;

public class Settings
{
    static final String TRACE_SCOPE = "Settings";

    public static final String TAG_URL = "URL";
    public static final String TAG_AUTODISCOVERY = "DISCOVERY";
    public static final boolean DEFAULT_AUTODISCOVERY = true;
    public static final String TAG_PORT = "PORT";
    public static final int DEFAULT_PORT = 1235;
    public static final String TAG_ENCRYPTION = "ENCRYPTION";
    public static final boolean DEFAULT_ENCRYPTION = false;
    public static final String TAG_PASSWORD = "PASSWORD";
    public static final String SETTINGS_FILE_NAME = "Settings";
    public static final String TAG_SAVE_LOG = "SAVE_LOG";
    public static final boolean DEFAULT_SAVE_LOG = false;

    String mUrl = "";
    boolean mAutoDiscovery = DEFAULT_AUTODISCOVERY;
    int mPort = DEFAULT_PORT;
    boolean mEncryptionStatus = DEFAULT_ENCRYPTION;
    String mPassword = "";
    boolean mSaveLog = DEFAULT_SAVE_LOG;

    public String getURL()
    {
        return mUrl;
    }
    public void setUrl(String url) { mUrl = url; }

    public int getPort()
    {
        return mPort;
    }

    public void setPort(int port)
    {
        mPort = port;
    }

    public boolean getAutoDiscovery()
    {
        return mAutoDiscovery;
    }

    public void setAutoDiscovery(boolean enable)
    {
        mAutoDiscovery = enable;
    }

    public boolean getEncryptionStatus()
    {
        return mEncryptionStatus;
    }

    public void setEncryptionStatus(boolean status)
    {
        mEncryptionStatus = status;
    }

    public String getEncryptionPassword()
    {
        return mPassword;
    }

    public void setEncryptionPassword(String password)
    {
        mPassword = password;
    }

    public boolean getSaveLog() { return mSaveLog; }
    public void setSaveLog(boolean saveLog) { mSaveLog = saveLog; }

    public void restoreFromBundle(@NonNull Bundle bundle)
    {
        mAutoDiscovery = bundle.getBoolean(TAG_AUTODISCOVERY, DEFAULT_AUTODISCOVERY);
        CharSequence url = bundle.getCharSequence(TAG_URL);
        if (url != null)
        {
            mUrl = url.toString();
        }
        mPort = bundle.getInt(TAG_PORT, DEFAULT_PORT);

        mEncryptionStatus = bundle.getBoolean(TAG_ENCRYPTION, DEFAULT_ENCRYPTION);

        CharSequence password = bundle.getCharSequence(TAG_PASSWORD);
        if (password != null)
        {
            mPassword = password.toString();
        }
        mSaveLog = bundle.getBoolean(TAG_SAVE_LOG, DEFAULT_SAVE_LOG);
    }

    public void saveToBundle(@NonNull Bundle bundle)
    {
        bundle.putString(TAG_URL, mUrl);
        bundle.putInt(TAG_PORT, mPort);
        bundle.putBoolean(TAG_AUTODISCOVERY, mAutoDiscovery);
        bundle.putBoolean(TAG_ENCRYPTION, mEncryptionStatus);
        bundle.putString(TAG_PASSWORD, mPassword);
        bundle.putBoolean(TAG_SAVE_LOG, mSaveLog);
    }

    public void restoreFromIntent(@NonNull Intent intent)
    {
        mPort = intent.getIntExtra(TAG_PORT, DEFAULT_PORT);
        mAutoDiscovery = intent.getBooleanExtra(TAG_AUTODISCOVERY, DEFAULT_AUTODISCOVERY);
        mUrl = intent.getStringExtra(TAG_URL);
        mEncryptionStatus = intent.getBooleanExtra(TAG_ENCRYPTION, DEFAULT_ENCRYPTION);
        mPassword = intent.getStringExtra(TAG_PASSWORD);
        mSaveLog = intent.getBooleanExtra(TAG_SAVE_LOG, DEFAULT_SAVE_LOG);
    }

    public void saveToIntent(@NonNull Intent intent)
    {
        intent.putExtra(TAG_URL, mUrl);
        intent.putExtra(TAG_PORT, mPort);
        intent.putExtra(TAG_AUTODISCOVERY, mAutoDiscovery);
        intent.putExtra(TAG_ENCRYPTION, mEncryptionStatus);
        intent.putExtra(TAG_PASSWORD, mPassword);
        intent.putExtra(TAG_SAVE_LOG, mSaveLog);
    }

    public void savePreferences(Context context)
    {
        try
        {
            EncryptedSharedPreferences sharedPreferences = (EncryptedSharedPreferences) EncryptedSharedPreferences.create(
                    SETTINGS_FILE_NAME,
                    MasterKeys.getOrCreate(MasterKeys.AES256_GCM_SPEC),
                    context,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
            );
            SharedPreferences.Editor editor = sharedPreferences.edit();
            editor.putString(TAG_URL, mUrl);
            editor.putInt(TAG_PORT, mPort);
            editor.putBoolean(TAG_AUTODISCOVERY, mAutoDiscovery);
            editor.putBoolean(TAG_ENCRYPTION, mEncryptionStatus);
            editor.putString(TAG_PASSWORD, mPassword);
            editor.putBoolean(TAG_SAVE_LOG, mSaveLog);
            editor.apply();
        }
        catch (GeneralSecurityException | IOException e)
        {
            Log.e(TRACE_SCOPE, "Failed to open encrypted shared preferences for writing: " + e.toString());
        }
    }

    public void restorePreferences(Context context)
    {
        try
        {
            EncryptedSharedPreferences sharedPreferences = (EncryptedSharedPreferences) EncryptedSharedPreferences.create(
                    SETTINGS_FILE_NAME,
                    MasterKeys.getOrCreate(MasterKeys.AES256_GCM_SPEC),
                    context,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
            );
            mUrl = sharedPreferences.getString(TAG_URL, "");
            mAutoDiscovery = sharedPreferences.getBoolean(TAG_AUTODISCOVERY, DEFAULT_AUTODISCOVERY);
            mPort = sharedPreferences.getInt(TAG_PORT, DEFAULT_PORT);
            mEncryptionStatus = sharedPreferences.getBoolean(TAG_ENCRYPTION, DEFAULT_ENCRYPTION);
            mPassword = sharedPreferences.getString(TAG_PASSWORD, "");
            mSaveLog = sharedPreferences.getBoolean(TAG_SAVE_LOG, DEFAULT_SAVE_LOG);
        }
        catch (GeneralSecurityException | IOException e)
        {
            Log.e(TRACE_SCOPE, "Failed to open encrypted shared preferences for reading: " + e.toString());
        }
    }

}
