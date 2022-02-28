//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <robuffer.h>
#include <string>
#include <Binarizer.h>
#include <CodaBarReader.h>

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
using namespace Windows::Storage;
using namespace Lumia::Imaging;
using namespace std;
using namespace Windows::Web::Http;
using namespace Windows::Phone::UI::Input;

#define filename "wp81CodeScannerFile.txt"
#define fileVersion "v1"

MainPage::MainPage()
	: _cameraPreviewImageSource(nullptr)
	, _bitmap(nullptr)
	, _bitmapRenderer(nullptr)
	, _isRendering(false)
	, pBufOrig(nullptr)
	, pBufDest(nullptr)
	, localFolder(nullptr)
{
	InitializeComponent();

	HardwareButtons::BackPressed += ref new EventHandler<BackPressedEventArgs ^>(this, &MainPage::HardwareButtons_BackPressed);

	// Useful to know when to initialize/clean up the camera
	_applicationSuspendingEventToken =
		Application::Current->Suspending += ref new SuspendingEventHandler(this, &MainPage::Application_Suspending);
	_applicationResumingEventToken =
		Application::Current->Resuming += ref new EventHandler<Object^>(this, &MainPage::Application_Resuming);

	localFolder = ApplicationData::Current->LocalFolder;
}

MainPage::~MainPage()
{
	Application::Current->Suspending -= _applicationSuspendingEventToken;
	Application::Current->Resuming -= _applicationResumingEventToken;
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

	ReadServerFile();

	Windows::Graphics::Display::DisplayInformation::AutoRotationPreferences = Windows::Graphics::Display::DisplayOrientations::Landscape;

	StartPreview();
}

void MainPage::HardwareButtons_BackPressed(Object^ sender, Windows::Phone::UI::Input::BackPressedEventArgs^ e)
{
	// Return to the list of computer.
	if (!IsStateScanner())
	{
		//Indicate the back button press is handled so the app does not exit
		e->Handled = true;

		StateScanner();
	}

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

void DebugGraph(uint8_t* pBufOrig, uint8_t* pBufDest, int width)
{
	for (int x = 0; x < width; x++) {
		if (x > 0) {
			// Luminance
			int y0 = 255 - *(pBufOrig + 360 * 1280 + x - 1) + 400;
			int y1 = 255 - *(pBufOrig + 360 * 1280 + x) + 400;
			if (y1 < y0) {
				int yTmp = y1;
				y1 = y0;
				y0 = yTmp;
			}

			for (int y = y0; y <= y1; y++)
			{
				*(pBufDest + x * 4 + y * 1280 * 4) = 0;
				*(pBufDest + x * 4 + y * 1280 * 4 + 1) = 0;
				*(pBufDest + x * 4 + y * 1280 * 4 + 2) = 255; // Red
			}
		}

		// Luminance threshold
		int yThrshld = 255 - *(pBufOrig + 361 * 1280 + x) + 400;
		*(pBufDest + x * 4 + yThrshld * 1280 * 4) = 0;
		*(pBufDest + x * 4 + yThrshld * 1280 * 4 + 1) = 255; // Green
		*(pBufDest + x * 4 + yThrshld * 1280 * 4 + 2) = 0;

		// 0
		*(pBufDest + x * 4 + 400 * 1280 * 4) = 255; // Blue
		*(pBufDest + x * 4 + 400 * 1280 * 4 + 1) = 0;
		*(pBufDest + x * 4 + 400 * 1280 * 4 + 2) = 0;

		// 255
		*(pBufDest + x * 4 + (400 + 255) * 1280 * 4) = 255; // Blue
		*(pBufDest + x * 4 + (400 + 255) * 1280 * 4 + 1) = 0;
		*(pBufDest + x * 4 + (400 + 255) * 1280 * 4 + 2) = 0;
	}
}

void Wp81CodeScanner::MainPage::AppBarButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
	Button^ b = (Button^)sender;
	if (b->Tag->ToString() == "Server")
	{
		// Want to configure the server.
		StateEditServer();
	}
	else if (b->Tag->ToString() == "Save")
	{
		// Save server URL
		WriteServerFile();
		StateScanner();
	}
	else if (b->Tag->ToString() == "Help")
	{
		// Want to see the help page.
		StateHelp();
	}
}

/**
* Median noise filter 3x3
*/
void Wp81CodeScanner::MainPage::NoiseFilter(uint8_t * pBufOrig, int row, int width)
{
	NoiseFilterX(pBufOrig, row - 1, width);
	NoiseFilterX(pBufOrig, row, width);
	NoiseFilterX(pBufOrig, row + 1, width);
	NoiseFilterY(pBufOrig, row, width);
}

/**
* Median noise filter X axis
*/
void Wp81CodeScanner::MainPage::NoiseFilterX(uint8_t * pBufOrig, int row, int width)
{
	int offset = row*width;
	uint8_t l1 = 0;
	uint8_t l2 = 0;
	uint8_t l3 = 0;
	uint8_t lTmp = 0;
	for (int x = 1; x < width - 1; x++) {
		l1 = *(pBufOrig + offset + x - 1);
		l2 = *(pBufOrig + offset + x);
		l3 = *(pBufOrig + offset + x + 1);

		if (l1 > l2) {
			lTmp = l2;
			l2 = l1;
			l1 = lTmp;
		}

		if (l1 > l3) {
			lTmp = l3;
			l3 = l1;
			l1 = lTmp;
		}

		if (l2 > l3) {
			lTmp = l3;
			l3 = l2;
			l2 = lTmp;
		}

		*(pBufOrig + offset + x) = l2;
	}
}

/**
* Median noise filter Y axis
*/
void Wp81CodeScanner::MainPage::NoiseFilterY(uint8_t * pBufOrig, int row, int width)
{
	int offset1 = (row - 1)*width;
	int offset2 = row*width;
	int offset3 = (row + 1)*width;
	uint8_t l1 = 0;
	uint8_t l2 = 0;
	uint8_t l3 = 0;
	uint8_t lTmp = 0;
	for (int x = 0; x < width; x++) {
		l1 = *(pBufOrig + offset1 + x);
		l2 = *(pBufOrig + offset2 + x);
		l3 = *(pBufOrig + offset3 + x);

		if (l1 > l2) {
			lTmp = l2;
			l2 = l1;
			l1 = lTmp;
		}

		if (l1 > l3) {
			lTmp = l3;
			l3 = l1;
			l1 = lTmp;
		}

		if (l2 > l3) {
			lTmp = l3;
			l3 = l2;
			l2 = lTmp;
		}

		*(pBufOrig + offset2 + x) = l2;
	}
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

void Wp81CodeScanner::MainPage::SuccessfulRead(std::string read)
{
	bool newRead = false;
	if (lastRead.compare(read) != 0) {
		// Totaly new read
		newRead = true;
		lastRead = read;
	}
	else if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastReadTime).count() > 1000) {
		// Same read but 5 seconds ago
		newRead = true;
	}

	if (newRead) {
		Debug("NEW READ\n");
		

		// transform string to wstring...
		std::wstring w_str = std::wstring(read.begin(), read.end());
		// ... and then to HttpStringContent
		HttpStringContent^ content = ref new HttpStringContent(ref new Platform::String(w_str.c_str()));

		String^ ServerURL = TextBoxURL->Text; //Example: L"http://192.168.1.30"
		if (ServerURL != nullptr) {
			Uri^ uri = ref new Uri(ServerURL);
			HttpClient^ httpClient = ref new HttpClient();
			create_task(httpClient->PostAsync(uri, content)).then([this](HttpResponseMessage^ httpResponse) {

				if (httpResponse->IsSuccessStatusCode) {
					Beep->Play();
				}
				else {
					Debug("POST response error: %d\n", httpResponse->StatusCode);
					std::wstring w_str = L"POST response error: " + to_wstring((int)httpResponse->StatusCode);
					TextBoxResult->Text = ref new Platform::String(w_str.c_str());
				}

			}).then([this](task<void> t) {
				try
				{
					// Try getting all exceptions from the continuation chain above this point.
					t.get();
				}
				catch (Exception^) {
					Debug("Exception.\n");
				}
				catch (std::exception &) {
					Debug("Exception.\n");
				}
				catch (...) {
					Debug("Exception.\n");
				}
			});
		}
	}
	lastReadTime = std::chrono::system_clock::now();
}

void Wp81CodeScanner::MainPage::Application_Suspending(Object ^ sender, Windows::ApplicationModel::SuspendingEventArgs ^ e)
{
	// Handle global application events only if this page is active
	if (Frame->CurrentSourcePageType.Name == Interop::TypeName(MainPage::typeid).Name)
	{
		// deferral = Ensure that the system lets the method complete before suspending the app.
		auto deferral = e->SuspendingOperation->GetDeferral();
		create_task(_cameraPreviewImageSource->StopPreviewAsync()).then([this, deferral]()
		{
			deferral->Complete();
		});
	}
}

void Wp81CodeScanner::MainPage::Application_Resuming(Object ^ sender, Object ^ args)
{
	// Handle global application events only if this page is active
	if (Frame->CurrentSourcePageType.Name == Interop::TypeName(MainPage::typeid).Name)
	{
		StartPreview();
	}
}

void MainPage::OnPreviewFrameAvailable(Lumia::Imaging::IImageSize ^imageSize)
{
	// Prevent multiple rendering attempts at once
	if (_isRendering == false)
	{
		_isRendering = true;

		//Debug("OnPreviewFrameAvailable %d\n",_bitmap);

		create_task(_bitmapRenderer->RenderAsync())
			.then([=](Bitmap^ bitmap) {
			create_task(Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler([=]()
			{
				// Improve low light perf
				NoiseFilter(pBufOrig, 360, 1280);

				try {
					Binarizer* binarizer(new Binarizer());
					// Binarize row 360 and store result in row 362 (row 361 contains the threshold used by the binarizer)
					binarizer->binarizeRow(pBufOrig + 360 * 1280, pBufOrig + 362 * 1280, pBufOrig + 361 * 1280, 1280);

					CodaBarReader* reader(new CodaBarReader());
					std::string result = reader->read(pBufOrig + 362 * 1280, 1280);
					Debug("Result: %s\n", result.c_str());
					std::wstring w_str = std::wstring(result.begin(), result.end());
					TextBoxResult->Text = ref new Platform::String(w_str.c_str());
					SuccessfulRead(result);
				}
				catch (char* reason) {
					Debug("Exception: %s\n", reason);
					string str(reason);
					std::wstring w_str = std::wstring(str.begin(), str.end());
					TextBoxResult->Text = ref new Platform::String(w_str.c_str());
				}
				

				// Transform 0..1 of row 362 to 0..255
				for (int x = 0; x < 1280; x++) {
					*(pBufOrig + 362 * 1280 + x) = *(pBufOrig + 362 * 1280 + x) * 255;
				}
				memcpy(pBufOrig + 363 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 364 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 365 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 366 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 367 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 368 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 369 * 1280, pBufOrig + 362 * 1280, 1280);
				memcpy(pBufOrig + 370 * 1280, pBufOrig + 362 * 1280, 1280);

				// Copy the luminance buffer of the preview to the BGRA buffer of the display (writeableBitmap)
				uint8_t* pBufDest2 = pBufDest;
				for (int i = 0; i < 1280*720; i++) {
					// Blue
					*pBufDest2 = *(pBufOrig + i);
					pBufDest2++;
					// Green
					*pBufDest2 = *(pBufOrig + i);
					pBufDest2++;
					// Red
					*pBufDest2 = *(pBufOrig + i);
					pBufDest2++;
					// Alpha
					*pBufDest2 = 0xFF;
					pBufDest2++;
				}

				DebugGraph(pBufOrig, pBufDest, 1280);

				previewImage->Source = _writeableBitmap;
				_writeableBitmap->Invalidate(); // force the PreviewBitmap to redraw

			}))).then([this]()
			{
				_isRendering = false;
			});

		});
	}
}

////////////////////////////////////////////////:


void Wp81CodeScanner::MainPage::StateHelp()
{
	PanelScanner->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	PanelEditServer->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	PanelHelp->Visibility = Windows::UI::Xaml::Visibility::Visible;
	ServerAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	HelpAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	SaveAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void Wp81CodeScanner::MainPage::StateScanner()
{
	PanelScanner->Visibility = Windows::UI::Xaml::Visibility::Visible;
	PanelEditServer->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	PanelHelp->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	ServerAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
	HelpAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
	SaveAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void Wp81CodeScanner::MainPage::StateEditServer()
{
	PanelScanner->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	PanelEditServer->Visibility = Windows::UI::Xaml::Visibility::Visible;
	PanelHelp->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	ServerAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	HelpAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	SaveAppBarButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

bool Wp81CodeScanner::MainPage::IsStateScanner()
{
	return PanelScanner->Visibility == Windows::UI::Xaml::Visibility::Visible;
}

void Wp81CodeScanner::MainPage::ReadServerFile()
{
	create_task(localFolder->GetFileAsync(filename)).then([this](StorageFile^ file)
	{
		return FileIO::ReadLinesAsync(file);
	}).then([this](IVector<String^>^ lines)
	{
		String^ version = lines->GetAt(0);
		Debug("File version "); OutputDebugString(version->Data()); Debug("\n");

		if (lines->Size > 1) {
			TextBoxURL->Text = lines->GetAt(1);
		}

	}).then([this](task<void> t)
	{

		try
		{
			t.get();
			// .get() didn' t throw, so we succeeded.
			Debug("Read file succeeded.\n");
		}
		catch (Platform::COMException^ e)
		{
			// The system cannot find the specified file.
			OutputDebugString(e->Message->Data());
			// First time that this application is launched?
			// Create empty file
			WriteServerFile();
		}

	});
}

void Wp81CodeScanner::MainPage::WriteServerFile()
{
	create_task(localFolder->CreateFileAsync(filename, CreationCollisionOption::ReplaceExisting)).then(
		[this](StorageFile^ file)
	{
		IVector<String^>^ lines = ref new Platform::Collections::Vector<String^>();
		lines->Append(fileVersion);

		if (TextBoxURL->Text != nullptr) {
			lines->Append(TextBoxURL->Text);
		}

		return FileIO::AppendLinesAsync(file, lines);
	});
}