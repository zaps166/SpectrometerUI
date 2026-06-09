package org.zaps166.UsbFD;
import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.Build;
import java.util.HashMap;

public class UsbFD
{
    private static final String ACTION_USB_PERMISSION = "org.zaps166.UsbFD.USB_PERMISSION";

    public native static void permissionsGrantedCallback(long ptr, boolean granted);

    private static UsbDeviceConnection mConnection = null;
    public static int getFD(Context context, long ptr, int vid, int pid)
    {
        boolean callCppCode = true;
        if (mConnection == null)
        {
            UsbManager manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
            for (UsbDevice device : manager.getDeviceList().values())
            {
                if (device.getVendorId() == vid && device.getProductId() == pid)
                {
                    if (manager.hasPermission(device))
                    {
                        mConnection = manager.openDevice(device);
                    }
                    else
                    {
                        int flags = (Build.VERSION.SDK_INT >= 33) ? Context.RECEIVER_NOT_EXPORTED : 0;
                        context.registerReceiver(
                            new BroadcastReceiver() {
                                public void onReceive(Context context, Intent intent)
                                {
                                    if (intent.getAction().equals(ACTION_USB_PERMISSION))
                                    {
                                        synchronized (this)
                                        {
                                            permissionsGrantedCallback(ptr, intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false));
                                            context.unregisterReceiver(this);
                                        }
                                    }
                                }
                            },
                            new IntentFilter(ACTION_USB_PERMISSION),
                            flags
                        );
                        Intent permIntent = new Intent(ACTION_USB_PERMISSION);
                        permIntent.setPackage(context.getPackageName());
                        manager.requestPermission(
                            device,
                            PendingIntent.getBroadcast(context, 0, permIntent, PendingIntent.FLAG_MUTABLE)
                        );
                        callCppCode = false;
                    }
                    break;
                }
            }
        }
        if (callCppCode)
        {
            permissionsGrantedCallback(ptr, false);
        }
        if (mConnection == null)
        {
            return callCppCode ? -1 : -2;
        }
        return mConnection.getFileDescriptor();
    }

    public static void close()
    {
        if (mConnection != null)
        {
            mConnection.close();
            mConnection = null;
        }
    }
}
