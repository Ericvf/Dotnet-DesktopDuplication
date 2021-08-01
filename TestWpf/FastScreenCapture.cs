using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace WpfApp1
{
    public class FastScreenCaptureService
    {
        private static class FastScreenCapture
        {
            [DllImport("FastScreenCapture.dll", CallingConvention = CallingConvention.Cdecl)]
            public static extern int Initialize(int outputNum, int width, int height);

            [DllImport("FastScreenCapture.dll")]
            public static extern void Clean();

            [DllImport("FastScreenCapture.dll", CallingConvention = CallingConvention.Cdecl)]
            public static extern int CaptureScreen(byte[] imageData, bool sbs, bool hou);
        }

        private static readonly FastScreenCaptureService instance = new FastScreenCaptureService();

        public static FastScreenCaptureService GetInstance()
        {
            return instance;
        }

        private FastScreenCaptureService()
        {

        }

        public event EventHandler Captured;
        CancellationTokenSource captureTaskToken;
        Task captureTask;

        private bool isSBS;
        public bool IsSBS
        {
            set
            {
                isSBS = value;
                if (isSBS && isHOU) isHOU = false;
            }
        }

        private bool isHOU;
        public bool IsHOU
        {
            set
            {
                isHOU = value;
                if (isSBS && isHOU) isSBS = false;
            }
        }

        private byte[] buffer;
        public byte[] GetBuffer => buffer;

        public void Start(int outputNum, int width, int height)
        {
            Stop();

            buffer = null;
            buffer = new byte[width * height * 4];

            FastScreenCapture.Initialize(outputNum, width, height);

            captureTaskToken = new CancellationTokenSource();
            captureTask = Task.Run(() => Capture(captureTaskToken.Token), captureTaskToken.Token);
        }

        public void Stop()
        {
            if (captureTask != null)
            {
                captureTaskToken.Cancel();
                captureTask.Wait();
                captureTask = null;
            }
        }

        private void Capture(CancellationToken captureTaskToken)
        {
            while (!captureTaskToken.IsCancellationRequested)
            {
                FastScreenCapture.CaptureScreen(buffer, isSBS, isHOU);
                Captured?.Invoke(this, null);
            }
        }

        public void Clean()
        {
            Stop();
            FastScreenCapture.Clean();
        }
    }
}
