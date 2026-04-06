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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.icu.lang.UProperty;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Switch;

import androidx.annotation.NonNull;

public class SettingsActivity extends Activity
{
    static final String TRACE_SCOPE = "SettingsActivity";

    Settings mSettings = new Settings();
    EditText mUrlEditor;
    EditText mPortEditor;
    RadioButton mDiscoveryRadioButton;
    RadioButton mConnectToRadioButton;
    @SuppressLint("UseSwitchCompatOrMaterialCode") Switch mEncryptionSwitch;
    @SuppressLint("UseSwitchCompatOrMaterialCode") Switch mSaveLogSwitch;
    EditText mPasswordEditor;
    Button mStartStreamingButton;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        mUrlEditor = findViewById(R.id.editTextURL);
        mPortEditor = findViewById(R.id.editTextDiscoveryPort);
        mDiscoveryRadioButton = findViewById(R.id.rbDiscovery);
        mConnectToRadioButton = findViewById(R.id.rbURL);
        mEncryptionSwitch = findViewById(R.id.switchUseEncryption);
        mPasswordEditor = findViewById(R.id.editTextPassword);
        mSaveLogSwitch = findViewById(R.id.switchSaveLog);
        mStartStreamingButton = findViewById(R.id.startStreaming);

        if (savedInstanceState != null)
        {
            mSettings.restoreFromBundle(savedInstanceState);
            Log.d(TRACE_SCOPE, "Restored settings from saved instance");
        }
        else
        {
            mSettings.restorePreferences(this);
            Log.d(TRACE_SCOPE, "Restored settings from secure storage");
        }
        setControlValues();
        setUIStateListeners();
    }

    @Override
    protected void onSaveInstanceState(@NonNull Bundle instanceState)
    {
        super.onSaveInstanceState(instanceState);
        mSettings.setUrl(mUrlEditor.getText().toString());
        mSettings.setPort(Integer.parseInt(mPortEditor.getText().toString()));
        mSettings.setAutoDiscovery(mDiscoveryRadioButton.isSelected());
        mSettings.setEncryptionStatus(mEncryptionSwitch.isChecked());
        mSettings.setEncryptionPassword(mPasswordEditor.getText().toString());
        mSettings.setSaveLog(mSaveLogSwitch.isChecked());

        mSettings.saveToBundle(instanceState);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        //  We need to override onResume to restore the natural orientation of this activity
        //  as it might get stuck in landscape after returning from StreamingActivity, which is always
        //  in landscape. The following line forces SettingsActivity to follow the orientation sensor
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
    }

    private void setControlValues()
    {
        mDiscoveryRadioButton.setSelected(mSettings.getAutoDiscovery());
        mConnectToRadioButton.setSelected(!mSettings.getAutoDiscovery());
        mUrlEditor.setText(mSettings.getURL());
        mPortEditor.setText(String.valueOf(mSettings.getPort()));
        mEncryptionSwitch.setChecked(mSettings.getEncryptionStatus());
        mPasswordEditor.setText(mSettings.getEncryptionPassword());
        mSaveLogSwitch.setChecked(mSettings.getSaveLog());
        updateControlStates();
    }

    private void updateControlStates()
    {
        boolean autoDiscovery = mSettings.getAutoDiscovery();
        mUrlEditor.setEnabled(!autoDiscovery);
        mPortEditor.setEnabled(autoDiscovery);
        mPasswordEditor.setEnabled(mSettings.getEncryptionStatus());
        boolean serverSpecified = (mSettings.getAutoDiscovery() && mSettings.getPort() != 0 || (!mSettings.getURL().isEmpty()));
        boolean encryptionValid = (!mSettings.getEncryptionStatus()) || (!mSettings.getEncryptionPassword().isEmpty());
        mStartStreamingButton.setEnabled(serverSpecified && encryptionValid);
    }

    private void setUIStateListeners()
    {
        mDiscoveryRadioButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                boolean discovery = isChecked;
                mSettings.setAutoDiscovery(discovery);
                if (discovery == false)
                {
                    mSettings.setUrl("");
                }
                updateControlStates();
            }
        });

        mConnectToRadioButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                mSettings.setAutoDiscovery(!isChecked);
                updateControlStates();
            }
        });

        mUrlEditor.addTextChangedListener(new TextWatcher()
        {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
                mSettings.setUrl(s.toString());
                updateControlStates();
            }

            @Override
            public void afterTextChanged(Editable s)
            {
            }
        });

        mPortEditor.addTextChangedListener(new TextWatcher()
        {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
                mSettings.setPort(Integer.parseInt(s.toString()));
                updateControlStates();
            }

            @Override
            public void afterTextChanged(Editable s)
            {
            }
        });

        mEncryptionSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                mSettings.setEncryptionStatus(isChecked);
                updateControlStates();
            }
        });
        mPasswordEditor.addTextChangedListener(new TextWatcher()
        {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
                mSettings.setEncryptionPassword(s.toString());
                updateControlStates();
            }

            @Override
            public void afterTextChanged(Editable s)
            {
            }
        });
        mSaveLogSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                mSettings.setSaveLog(isChecked);
            }
        });

        mStartStreamingButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                mSettings.savePreferences(SettingsActivity.this);
                Log.d(TRACE_SCOPE, "Saved settings to secure storage");
                Log.d(TRACE_SCOPE, "Preparing to launch streaming activity");
                Intent intent = new Intent(SettingsActivity.this, StreamingActivity.class);
                intent.setAction(StreamingActivity.ACTION_START);
                mSettings.saveToIntent(intent);
                startActivity(intent);
                Log.d(TRACE_SCOPE, "Started streaming activity");
            }
        });
    }
}
