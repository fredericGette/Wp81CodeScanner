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
		Lumia::Imaging::CameraPreviewImageSource^ _cameraPreviewImageSource;
		bool _isRendering;
		Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ _writeableBitmap;
		Lumia::Imaging::FilterEffect^ _effect;
		Lumia::Imaging::WriteableBitmapRenderer^ _writeableBitmapRenderer; // renderer for our images

		void Button_Start_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Button_Capture_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Button_Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnPreviewFrameAvailable(Lumia::Imaging::IImageSize ^imageSize);
	};
}
