//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace Wp81CodeScanner
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		// Prevent the screen from sleeping while the camera is running
		Windows::System::Display::DisplayRequest^ _displayRequest;

		// MediaCapture
		Platform::Agile<Windows::Media::Capture::MediaCapture^> _mediaCapture;
		Platform::Agile<Windows::Media::Capture::LowLagPhotoCapture^> _photoCapture;
		Windows::Storage::Streams::IBuffer^ readBuffer;

		void Button_Start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Button_Capture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void capturePhoto();
		void Button_Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
