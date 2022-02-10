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
using namespace ZXing;
using namespace std;


// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
	: _cameraPreviewImageSource(nullptr)
	, _isRendering(false)
	, frameCounter(0)
	, _reader(nullptr)
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

void MainPage::Button_Start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
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
				
				_reader = ref new BarcodeReader();
				//reader->AutoRotate = true;
				ZXing::Common::DecodingOptions^ options = ref new ZXing::Common::DecodingOptions();
				//options->TryHarder = true;
				options->PossibleFormats = ref new Platform::Array<ZXing::BarcodeFormat>(1);
				options->PossibleFormats[0] = ZXing::BarcodeFormat::QR_CODE;
				_reader->Options = options;

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

		//Debug("OnPreviewFrameAvailable\n");

		create_task(_cameraPreviewImageSource->GetBitmapAsync(nullptr, OutputOption::PreserveSize))
			.then([=](Bitmap^ bitmap) {
		//create_task(_writeableBitmapRenderer->RenderAsync())
		//	.then([=](WriteableBitmap^ wBitmap) {
			//Debug("RenderAsync\n");
			create_task(Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler([=]()
			{
				//Debug("callBack\n");
				//Debug("Width %d\n", bitmap->Dimensions.Width);
				//Debug("Number of buffers %d\n", bitmap->Buffers->Length);
				//Debug("Pitch %d\n", bitmap->Buffers->get(0)->Pitch);
				// buffer 0: Y
				// buffer 1: UV
				//Debug("ColorMode %d\n", bitmap->Buffers->get(0)->ColorMode);
				IBuffer^ buffer = bitmap->Buffers->get(0)->Buffer;

				::IUnknown* pUnkOrig{ reinterpret_cast<IUnknown*>(buffer) };
				Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccessOrig;
				HRESULT hrOrig{ pUnkOrig->QueryInterface(IID_PPV_ARGS(&bufferByteAccessOrig)) };
				byte *pBufOrig{ nullptr };
				bufferByteAccessOrig->Buffer(&pBufOrig);

				::IUnknown* pUnkDest{ reinterpret_cast<IUnknown*>(_writeableBitmap->PixelBuffer) };
				Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccessDest;
				HRESULT hrDest{ pUnkDest->QueryInterface(IID_PPV_ARGS(&bufferByteAccessDest)) };
				byte *pBufDest{ nullptr };
				bufferByteAccessDest->Buffer(&pBufDest);
				for (int i = 0; i < 1280*720; i++) {
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

				Platform::Array<unsigned char>^ arrByte = ref new Platform::Array<unsigned char>(pBufOrig, 1280 * 720);
				Result^ result = _reader->Decode(arrByte, 1280,720, BitmapFormat::Gray8);
				wstring text = L"";
				if (result) {
					text = text + to_wstring(frameCounter) + L" Result " + result->Text->Data() + L" BarcodeFormat " + to_wstring((int)result->BarcodeFormat) + L"\n";
				}
				else {
					text = text + to_wstring(frameCounter) + L" No result\n";
				}
				OutputDebugString(text.c_str());
				TextBoxResult->Text = ref new String(text.c_str());

				previewImage->Source = _writeableBitmap;
				_writeableBitmap->Invalidate(); // force the PreviewBitmap to redraw

			}))).then([this]()
			{
				_isRendering = false;
			});

		});
	}
}


