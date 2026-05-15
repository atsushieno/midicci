package dev.celtera.libremidi;

import android.media.midi.MidiReceiver;
import java.io.IOException;

public class MidiUmpReceiver extends MidiReceiver {
    private long nativePtr;

    public MidiUmpReceiver(long ptr) {
        nativePtr = ptr;
    }

    @Override
    public void onSend(byte[] msg, int offset, int count, long timestamp) throws IOException {
        if (nativePtr != 0)
            onUmpData(nativePtr, msg, offset, count, timestamp);
    }

    public void deactivate() {
        nativePtr = 0;
    }

    private native void onUmpData(long ptr, byte[] msg, int offset, int count, long timestamp);
}
