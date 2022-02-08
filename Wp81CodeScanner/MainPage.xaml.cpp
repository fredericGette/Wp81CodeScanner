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
using namespace Lumia::Imaging;


// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage() 
	: _cameraPreviewImageSource(nullptr)
	, _isRendering(false)
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
				_effect = ref new FilterEffect(_cameraPreviewImageSource);
				_writeableBitmapRenderer = ref new WriteableBitmapRenderer(_effect, _writeableBitmap);

				_cameraPreviewImageSource->PreviewFrameAvailable += ref new Lumia::Imaging::PreviewFrameAvailableDelegate(this, &MainPage::OnPreviewFrameAvailable);
			});
		});

}

void MainPage::Button_Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	create_task(_cameraPreviewImageSource->StopPreviewAsync());
}

void MainPage::Button_Capture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

}

void MainPage::OnPreviewFrameAvailable(Lumia::Imaging::IImageSize ^imageSize)
{
	// Prevent multiple rendering attempts at once
	if (_isRendering == false)
	{
		_isRendering = true;

		Debug("OnPreviewFrameAvailable\n");

		create_task(_writeableBitmapRenderer->RenderAsync())
			.then([=](WriteableBitmap^ wBitmap) {
			Debug("RenderAsync\n");
			create_task(Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler([this]()
			{
				Debug("callBack\n");
				Debug("Width %d\n", _writeableBitmap->PixelWidth);
				previewImage->Source = _writeableBitmap; // previewImage is an image element in MainPage.xaml
				_writeableBitmap->Invalidate(); // force the PreviewBitmap to redraw
			}))).then([this]()
			{
				_isRendering = false;
			});

		});
	}
}


