//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include <chrono>

namespace Wp81CodeScanner
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		virtual ~MainPage();
		void HardwareButtons_BackPressed(Object ^ sender, Windows::Phone::UI::Input::BackPressedEventArgs ^ e);

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void AppBarButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		Lumia::Imaging::CameraPreviewImageSource^ _cameraPreviewImageSource;
		Lumia::Imaging::BitmapRenderer^ _bitmapRenderer;
		bool _isRendering;
		Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ _writeableBitmap;
		Lumia::Imaging::Bitmap^ _bitmap;
		byte *pBufOrig;
		byte *pBufDest;
		void NoiseFilter(uint8_t* pBufOrig, int row, int width);
		void NoiseFilterX(uint8_t* pBufOrig, int row, int width);
		void NoiseFilterY(uint8_t* pBufOrig, int row, int width);
		std::string lastRead;
		std::chrono::system_clock::time_point lastReadTime;
		void SuccessfulRead(std::string read);

		// Event tokens
		Windows::Foundation::EventRegistrationToken _applicationSuspendingEventToken;
		Windows::Foundation::EventRegistrationToken _applicationResumingEventToken;

		// Event handlers
		void Application_Suspending(Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void Application_Resuming(Object^ sender, Object^ args);
		void StateHelp();
		void StateScanner();
		void StateEditServer();
		bool IsStateScanner();
		void ReadServerFile();
		void WriteServerFile();
		void OnPreviewFrameAvailable(Lumia::Imaging::IImageSize ^imageSize);

		void StartPreview();

		// Local storage folder.
		Windows::Storage::StorageFolder^ localFolder;
		
	};
}
