//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <robuffer.h>
#include <string>

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
using namespace Lumia::Imaging;
using namespace zxing;
using namespace std;

Ref<Reader> reader;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
	: _cameraPreviewImageSource(nullptr)
	, _bitmap(nullptr)
	, _bitmapRenderer(nullptr)
	, _isRendering(false)
	, frameCounter(0)
	, pBufOrig(nullptr)
	, pBufDest(nullptr)
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

	//https://stackoverflow.com/questions/27394751/how-to-get-preview-buffer-of-mediacapture-universal-app
	//https://stackoverflow.com/questions/29947225/access-preview-frame-from-mediacapture

	Windows::Graphics::Display::DisplayInformation::AutoRotationPreferences = Windows::Graphics::Display::DisplayOrientations::Landscape;

	StartPreview();
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

void MainPage::StartPreview()
{
	_cameraPreviewImageSource = ref new CameraPreviewImageSource();
	create_task(_cameraPreviewImageSource->InitializeAsync(""))
		.then([=](task<void> previousTask) {
			previousTask.get();

			create_task(_cameraPreviewImageSource->StartPreviewAsync())
				.then([=](MediaProperties::VideoEncodingProperties^ vep) {

				Debug("Biterate %d\n", vep->Bitrate);
				Debug("FrameRate %d/%d\n", vep->FrameRate->Numerator, vep->FrameRate->Denominator);
				Debug("Width %d\n", vep->Width);
				Debug("Height %d\n", vep->Height);
				Debug("Type ");OutputDebugString(vep->Type->Data()); Debug("\n");
				Debug("Subtype "); OutputDebugString(vep->Subtype->Data()); Debug("\n");


				_writeableBitmap = ref new WriteableBitmap(vep->Width, vep->Height);
				_bitmap = ref new Bitmap(Size(1280, 720), ColorMode::Yuv420Sp);
				_bitmapRenderer = ref new BitmapRenderer(_cameraPreviewImageSource, _bitmap);

				IBuffer^ buffer = _bitmap->Buffers->get(0)->Buffer;
				::IUnknown* pUnkOrig{ reinterpret_cast<IUnknown*>(buffer) };
				Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccessOrig;
				HRESULT hrOrig{ pUnkOrig->QueryInterface(IID_PPV_ARGS(&bufferByteAccessOrig)) };
				bufferByteAccessOrig->Buffer(&pBufOrig);

				::IUnknown* pUnkDest{ reinterpret_cast<IUnknown*>(_writeableBitmap->PixelBuffer) };
				Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccessDest;
				HRESULT hrDest{ pUnkDest->QueryInterface(IID_PPV_ARGS(&bufferByteAccessDest)) };
				bufferByteAccessDest->Buffer(&pBufDest);

				
				
				_cameraPreviewImageSource->PreviewFrameAvailable += ref new Lumia::Imaging::PreviewFrameAvailableDelegate(this, &MainPage::OnPreviewFrameAvailable);
				
				Windows::Media::Devices::VideoDeviceController^ vdc = (Windows::Media::Devices::VideoDeviceController^)_cameraPreviewImageSource->VideoDeviceController;
				Windows::Media::Devices::FocusControl^ fc = vdc->FocusControl;
				Windows::Media::Devices::FocusSettings ^fs = ref new Windows::Media::Devices::FocusSettings();
				// Manual minimal focus distance
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
					});
				});
			});
		});

}

void MainPage::Button_Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	create_task(_cameraPreviewImageSource->StopPreviewAsync());
}

void MainPage::OnPreviewFrameAvailable(Lumia::Imaging::IImageSize ^imageSize)
{
	frameCounter++;

	// Prevent multiple rendering attempts at once
	if (_isRendering == false)
	{
		_isRendering = true;

		//Debug("OnPreviewFrameAvailable %d\n",_bitmap);

		create_task(_bitmapRenderer->RenderAsync())
			.then([=](Bitmap^ bitmap) {
		//create_task(_cameraPreviewImageSource->GetBitmapAsync(nullptr, OutputOption::PreserveSize))
		//	.then([=](Bitmap^ bitmap) {
		//create_task(_writeableBitmapRenderer->RenderAsync())
		//	.then([=](WriteableBitmap^ wBitmap) {
			//Debug("RenderAsync\n");
			create_task(Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler([=]()
			{
				//Debug("callBack\n");
				//Debug("Width %f\n", bitmap->Dimensions.Width);
				//Debug("Height %f\n", bitmap->Dimensions.Height);
				//Debug("Number of buffers %d\n", bitmap->Buffers->Length);
				//Debug("Pitch %d\n", bitmap->Buffers->get(0)->Pitch);
				// buffer 0: Y
				// buffer 1: UV
				//Debug("ColorMode buf0 %d\n", bitmap->Buffers->get(0)->ColorMode);
				//Debug("Pitch buf0 %d\n", bitmap->Buffers->get(0)->Pitch);
				//Debug("Capacity buf0 %d\n", bitmap->Buffers->get(0)->Buffer->Capacity);
				//Debug("ColorMode buf1 %d\n", bitmap->Buffers->get(1)->ColorMode);
				//Debug("Pitch buf1 %d\n", bitmap->Buffers->get(1)->Pitch);
				//Debug("Capacity buf1 %d\n", bitmap->Buffers->get(1)->Buffer->Capacity);

				BYTE *pBufOrig2 = pBufOrig;
				BYTE *pBufDest2 = pBufDest;
				for (int i = 0; i < 1280*720; i++) {
					// Blue
					*pBufDest2 = *(pBufOrig2 + i);
					pBufDest2++;
					// Green
					*pBufDest2 = *(pBufOrig2 + i);
					pBufDest2++;
					// Red
					*pBufDest2 = *(pBufOrig2 + i);
					pBufDest2++;
					// Alpha
					*pBufDest2 = 0xFF;
					pBufDest2++;
				}
				char *test = (char*)pBufOrig;
				ArrayRef<char> test2 = zxing::ArrayRef<char>(test, 1280*720);

				Ref<LuminanceSource> source(new GreyscaleLuminanceSource(test2, 1280, 720, 0, 0, 1280, 720));
				Ref<Binarizer> binarizer(new HybridBinarizer(source));

				Ref<BinaryBitmap> image(new BinaryBitmap(binarizer));
				DecodeHints hints(DecodeHints::CODE_128_HINT);

				reader = new zxing::oned::MultiFormatOneDReader(hints);
				try {
					Ref<Result> result = reader->decode(image, hints);
					Ref<zxing::String> text = result->getText();
					
					Debug("Result: %s\n", text->getText().c_str());
					std::string sTxt =text->getText();
					std::wstring w_str = to_wstring(frameCounter) + L" " + std::wstring(sTxt.begin(), sTxt.end());
					TextBoxResult->Text = ref new Platform::String(w_str.c_str());
				}
				catch (zxing::NotFoundException const& ignored) {
					(void)ignored;
					Debug("NotFoundException\n");
				}

				previewImage->Source = _writeableBitmap;
				_writeableBitmap->Invalidate(); // force the PreviewBitmap to redraw

			}))).then([this]()
			{
				_isRendering = false;
			});

		});
	}
}


