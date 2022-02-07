//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <robuffer.h>

using namespace Wp81CodeScanner;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Media::Capture;
using namespace Windows::Media;
using namespace concurrency;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Storage::Streams;


// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage() 
	: _mediaCapture(nullptr)
	, _displayRequest(ref new Windows::System::Display::DisplayRequest())
{
	InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter

	// TODO: Prepare page for display here.

	// TODO: If your application contains multiple pages, ensure that you are
	// handling the hardware Back button by registering for the
	// Windows::Phone::UI::Input::HardwareButtons.BackPressed event.
	// If you are using the NavigationHelper provided by some templates,
	// this event is handled for you.

	// https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/HolographicMixedRealityCapture/cpp/MediaCaptureManager.cpp
	// https://github.com/Microsoft/Windows-universal-samples/blob/main/Samples/HolographicFaceTracking/cpp/Content/VideoFrameProcessor.cpp
	// https://stackoverflow.com/questions/44468297/how-to-know-that-camera-is-currently-being-used-by-another-application-with-uwp
	// https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/CameraGetPreviewFrame/cpp/MainPage.xaml.cpp
	// https://docs.microsoft.com/en-us/previous-versions/windows/apps/dn642092(v=win.10)

}

void Debug(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[100];
	vsnprintf_s(buffer, sizeof(buffer), format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

void MainPage::Button_Start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	_mediaCapture = ref new Windows::Media::Capture::MediaCapture();
	MediaCaptureInitializationSettings^ settings = ref new MediaCaptureInitializationSettings();
//	settings->StreamingCaptureMode = StreamingCaptureMode::Video;
//	settings->PhotoCaptureSource = PhotoCaptureSource::VideoPreview;

	create_task(_mediaCapture->InitializeAsync(settings))
		.then([this](task<void> previousTask) {
		try
		{
			previousTask.get();
			_displayRequest->RequestActive();
			//Windows::Graphics::Display::DisplayInformation::AutoRotationPreferences = Windows::Graphics::Display::DisplayOrientations::Landscape;
			PreviewControl->Source = _mediaCapture.Get();

			//https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/get-a-preview-frame
			Windows::Media::MediaProperties::VideoEncodingProperties^ previewProperties = (Windows::Media::MediaProperties::VideoEncodingProperties^)_mediaCapture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview);
			Debug("*********** test ****************\n"); 
			Debug("Width: %d\n", previewProperties->Width);
			Debug("Height: %d\n", previewProperties->Height);

			create_task(_mediaCapture->StartPreviewAsync())
				.then([this](task<void>& previousTask)
			{
				try
				{
					previousTask.get();

					Debug("FocusControl Supported: %d\n", _mediaCapture->VideoDeviceController->FocusControl->Supported);
					Windows::Media::Devices::FocusControl^ fc = _mediaCapture->VideoDeviceController->FocusControl;
					Windows::Media::Devices::FocusSettings ^fs = ref new Windows::Media::Devices::FocusSettings();
					// Manual macro focus
					fs->Mode = Windows::Media::Devices::FocusMode::Manual;
					fs->DisableDriverFallback = true;
					fs->Value = fc->Min;
					create_task(fc->LockAsync())
						.then([=](task<void>& previousTask)
					{
						previousTask.get();
						fc->Configure(fs);
						create_task(fc->FocusAsync())
							.then([=](task<void>& previousTask)
						{
							previousTask.get();
							//Windows::Media::MediaProperties::ImageEncodingProperties^ imgEncProp = Windows::Media::MediaProperties::ImageEncodingProperties::CreateUncompressed(Windows::Media::MediaProperties::MediaPixelFormat::Bgra8);
							// https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering?redirectedfrom=MSDN
							Windows::Media::MediaProperties::ImageEncodingProperties^ imgEncProp = Windows::Media::MediaProperties::ImageEncodingProperties::CreateUncompressed(Windows::Media::MediaProperties::MediaPixelFormat::Nv12);
							imgEncProp->Width = 320;
							imgEncProp->Height = 240;
							create_task(_mediaCapture->PrepareLowLagPhotoCaptureAsync(imgEncProp))
								.then([this](LowLagPhotoCapture^ photoCapture)
							{
								_photoCapture = photoCapture;
							});
						});
					});
				}
				catch (Exception^ exception)
				{
					if (exception->HResult == 0x80070020)
					{
						auto messageDialog = ref new Windows::UI::Popups::MessageDialog("Cannot setup camera; currently being using.");
						create_task(messageDialog->ShowAsync());
					}
				}
			});
		}
		catch (AccessDeniedException^)
		{
			auto messageDialog = ref new Windows::UI::Popups::MessageDialog("The app was denied access to the camera.");
			create_task(messageDialog->ShowAsync());
		}
	});
}

void MainPage::Button_Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	create_task(_photoCapture->FinishAsync());
	create_task(_mediaCapture->StopPreviewAsync());
	PreviewControl->Source = nullptr;
	_displayRequest->RequestRelease();
	_mediaCapture = nullptr;
}

void MainPage::Button_Capture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

	create_task(_photoCapture->CaptureAsync())
		.then([this](CapturedPhoto^ photo)
	{
		Debug("Frame Width: %d\n", photo->Frame->Width);
		Debug("Frame Height: %d\n", photo->Frame->Height);
		Debug("Frame Type:"); OutputDebugString(photo->Frame->ContentType->Data()); Debug("\n");
		Debug("Frame CanRead: %d\n", photo->Frame->CanRead);
		Debug("Frame CanWrite: %d\n", photo->Frame->CanWrite);
		Debug("Frame Position: %d\n", photo->Frame->Position);
		Debug("Frame Size: %d\n", photo->Frame->Size);
		int width = photo->Frame->Width;
		int height = photo->Frame->Height;

		readBuffer = ref new Buffer(width*height*4); // Pixel format Bgra8
		create_task(photo->Frame->ReadAsync(readBuffer, readBuffer->Capacity, InputStreamOptions::Partial))
			.then([=](task<IBuffer^> readTask) 
		{
			IBuffer^ bufOrig = readTask.get();
			Debug("Bytes read from stream : %d\n", bufOrig->Length);

			::IUnknown* pUnkOrig{ reinterpret_cast<IUnknown*>(bufOrig) };
			Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccessOrig;
			HRESULT hrOrig{ pUnkOrig->QueryInterface(IID_PPV_ARGS(&bufferByteAccessOrig)) };
			byte *pBufOrig{ nullptr };
			bufferByteAccessOrig->Buffer(&pBufOrig);

			/*byte *pBufferCpy = pBuffer;
			*pBufferCpy = 0xFF; ++pBufferCpy;
			*pBufferCpy = 0xFF; ++pBufferCpy;
			*pBufferCpy = 0x0; ++pBufferCpy;
			*pBufferCpy = 0x0;*/

			//FillMemory(pBuffer + 320 * 4 * 120, 320*4, 0);

			//Debug("PixelBuffer 0 BGRA8 0x%02X%02X%02X%02X\n", *(pBufOrig +0), *(pBufOrig +1), *(pBufOrig +2), *(pBufOrig +3));
			//Debug("PixelBuffer 1000 BGRA8 0x%02X%02X%02X%02X\n", *(pBufOrig +1000), *(pBufOrig + 1001), *(pBufOrig + 1002), *(pBufOrig + 1003));
			//Debug("PixelBuffer 100000 BGRA8 0x%02X%02X%02X%02X\n", *(pBufOrig + 100000), *(pBufOrig + 100001), *(pBufOrig + 100002), *(pBufOrig + 100003));


			WriteableBitmap ^ bImg = ref new WriteableBitmap(width, height);
			Debug("PixelBuffer Length: %d\n", bImg->PixelBuffer->Length);

			::IUnknown* pUnkDest{ reinterpret_cast<IUnknown*>(bImg->PixelBuffer) };
			Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccessDest;
			HRESULT hrDest{ pUnkDest->QueryInterface(IID_PPV_ARGS(&bufferByteAccessDest)) };
			byte *pBufDest{ nullptr };
			bufferByteAccessDest->Buffer(&pBufDest);

			for (int i = 0; i < width*height; i++) {
				// Blue
				*pBufDest = *(pBufOrig + i);
				pBufDest++;
				// Green
				*pBufDest = *(pBufOrig + i);
				pBufDest++;
				// Red
				*pBufDest = *(pBufOrig + i);
				pBufDest++;
				// Alpha
				*pBufDest = 0xFF;
				pBufDest++;
			}
			//memcpy_s(pBuffer2, bImg->PixelBuffer->Length, pBuffer, buffer->Length);

			ImageControl->Source = bImg;
		});

	});
}
