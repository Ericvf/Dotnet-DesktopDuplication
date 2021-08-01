using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace DllImportTest
{
    class Program
    {
        static void Main()
        {
            FastScreenCapture.Initialize(0);
            var bytes = new byte[3840 * 2160 * 4];

            var sw = new Stopwatch();
            sw.Start();

            int c = 0;
            while (true)
            {
                FastScreenCapture.CaptureScreen(bytes);
                c++;
                if (sw.ElapsedMilliseconds > 1000)
                {
                    sw.Restart();
                    Console.WriteLine(c);
                    c = 0;
                }
            }

            //FastScreenCapture.Clean();
        }
    }

    public static class FastScreenCapture
    {
        [DllImport("FastScreenCapture.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Initialize(int outputNum);

        [DllImport("FastScreenCapture.dll")]
        public static extern void Clean();

        [DllImport("FastScreenCapture.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int CaptureScreen(byte[] imageData);
    }
}
