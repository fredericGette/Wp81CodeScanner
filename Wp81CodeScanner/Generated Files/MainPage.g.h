﻿

#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------

namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class StackPanel;
                ref class TextBox;
                ref class MediaElement;
                ref class Image;
                ref class AppBarButton;
            }
        }
    }
}

namespace Wp81CodeScanner
{
    partial ref class MainPage : public ::Windows::UI::Xaml::Controls::Page, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
    
        private: ::Windows::UI::Xaml::Controls::StackPanel^ PanelScanner;
        private: ::Windows::UI::Xaml::Controls::StackPanel^ PanelEditServer;
        private: ::Windows::UI::Xaml::Controls::StackPanel^ PanelHelp;
        private: ::Windows::UI::Xaml::Controls::TextBox^ TextBoxURL;
        private: ::Windows::UI::Xaml::Controls::MediaElement^ Beep;
        private: ::Windows::UI::Xaml::Controls::TextBox^ TextBoxReader;
        private: ::Windows::UI::Xaml::Controls::TextBox^ TextBoxNetwork;
        private: ::Windows::UI::Xaml::Controls::Image^ previewImage;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ ServerAppBarButton;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ SaveAppBarButton;
        private: ::Windows::UI::Xaml::Controls::AppBarButton^ HelpAppBarButton;
    };
}

