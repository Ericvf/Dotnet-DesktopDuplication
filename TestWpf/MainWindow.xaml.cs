using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace WpfApp1
{
    public partial class MainWindow : Window
    {
        int width = 512;
        int height = 512;
        int screen = 0;

        WriteableBitmap writeableBitmap;

        Stopwatch fpsTimer = new Stopwatch();
        int fpsCounter = 0;

        FastScreenCaptureService captureService = FastScreenCaptureService.GetInstance();

        public MainWindow()
        {
            InitializeComponent();

            RenderOptions.SetBitmapScalingMode(img, BitmapScalingMode.NearestNeighbor);

            writeableBitmap = new WriteableBitmap(width, height, 96, 96, PixelFormats.Bgra32, null);

            fpsTimer.Start();

            captureService.Captured += Service_Captured;
            captureService.Start(screen, width, height);

            KeyUp += MainWindow_KeyUp;

            CompositionTarget.Rendering += CompositionTarget_Rendering;

            Closed += MainWindow_Closed;
        }

        private void MainWindow_Closed(object sender, EventArgs e)
        {
            captureService.Clean();
        }

        private void Service_Captured(object sender, EventArgs e)
        {
            Dispatcher.BeginInvoke(new Action(() =>
            {
                writeableBitmap.WritePixels(new Int32Rect(0, 0, width, height),
                    captureService.GetBuffer, width * 4, 0);

                fpsCounter++;
                if (fpsTimer.ElapsedMilliseconds > 1000)
                {
                    this.Title = fpsCounter.ToString();
                    fpsCounter = 0;
                    fpsTimer.Restart();
                }

            }));
        }

        private void MainWindow_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key >= Key.F1 && e.Key <= Key.F12)
            {
                e.Handled = true;
                captureService.Stop();
                
                switch (e.Key)
                {
                    case Key.F1: screen = 0; break;
                    case Key.F2: screen = 1; break;
                    case Key.F3: width = 16; height = 16; break;
                    case Key.F4: width = 64; height = 64; break;
                    case Key.F5: width = 256; height = 256; break;
                    case Key.F6: width = 1024; height = 1024; break;
                    case Key.F7: width = 1920; height = 1080; break;
                    case Key.F8: width = 3840; height = 2160; break;
                    case Key.F9: captureService.IsSBS = captureService.IsHOU = false; break;
                    case Key.F11: captureService.IsSBS = true; break;
                    case Key.F12: captureService.IsHOU = true; break;
                    default:
                        break;
                }


                writeableBitmap = new WriteableBitmap(width, height, 96, 96, PixelFormats.Bgra32, null);
                captureService.Start(screen, width, height);
            }
        }

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
            img.Source = writeableBitmap;
        }

    }
}

