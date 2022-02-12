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
		Lumia::Imaging::BitmapRenderer^ _bitmapRenderer;
		bool _isRendering;
		Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ _writeableBitmap;
		Lumia::Imaging::Bitmap^ _bitmap;
		unsigned char frameCounter;
		byte *pBufOrig;
		byte *pBufDest;

		void StartPreview();
		void Button_Stop_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnPreviewFrameAvailable(Lumia::Imaging::IImageSize ^imageSize);
	};
}
