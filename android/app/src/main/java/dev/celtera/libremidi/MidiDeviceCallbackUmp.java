package dev.celtera.libremidi;

import android.media.midi.MidiDevice;
import android.media.midi.MidiManager;
import android.util.Log;

public class MidiDeviceCallbackUmp implements MidiManager.OnDeviceOpenedListener {
    private static final String TAG = "libremidi";
    private long nativePtr;
    private boolean isOutput;

    public MidiDeviceCallbackUmp(long ptr, boolean output) {
        nativePtr = ptr;
        isOutput = output;
    }

    @Override
    public void onDeviceOpened(MidiDevice device) {
        if (device == null) {
            Log.e(TAG, "Failed to open UMP MIDI device");
            return;
        }
        onDeviceOpened(device, nativePtr, isOutput);
    }

    private native void onDeviceOpened(MidiDevice device, long targetPtr, boolean isOutput);
}
