#pragma warning(disable : 4251)
#pragma warning(disable : 4996)
#include "hsn_usb_plugin.h"
#include "include/HsnLibrary.hpp"
#include "include/USBReceiver.hpp"
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
#include <memory>
#include <sstream>

#ifdef _WIN32
#include <dbt.h>
#include <tchar.h>
#endif

template <typename T = flutter::EncodableValue>
class HsnUSBStreamHandler : public flutter::StreamHandler<T>
{
public:
    HsnUSBStreamHandler() = default;
    virtual ~HsnUSBStreamHandler() = default;
    void onCallback(flutter::EncodableValue _data)
    {
        std::unique_lock<std::mutex> _ul(m_mtx);
        if (m_sink.get())
            m_sink.get()->Success(_data);
    }

protected:
    std::unique_ptr<flutter::StreamHandlerError<T>> OnListenInternal(const T *arguments, std::unique_ptr<flutter::EventSink<T>> &&events) override
    {
        std::unique_lock<std::mutex> _ul(m_mtx);
        m_sink = std::move(events);
        return nullptr;
    }
    std::unique_ptr<flutter::StreamHandlerError<T>> OnCancelInternal(const T *arguments) override
    {
        std::unique_lock<std::mutex> _ul(m_mtx);
        m_sink.release();
        return nullptr;
    }

private:
    flutter::EncodableValue m_value;
    std::mutex m_mtx;
    std::unique_ptr<flutter::EventSink<T>> m_sink;
};

class HsnUSBPlugin : public flutter::Plugin
{
public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);
    HsnUSBPlugin(flutter::PluginRegistrarWindows *registrar);
    virtual ~HsnUSBPlugin();

private:
    void HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &method_call, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    std::optional<LRESULT> HandleWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    std::string HsnUSBPlugin::connectUSB(const int &result);
    flutter::PluginRegistrarWindows *registrar;
    std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>> m_event_channel;
    HsnUSBStreamHandler<> *m_handler;
    int window_proc_id = -1;
};

void HsnUSBPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar)
{
    HsnUSBPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarManager::GetInstance()
            ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

void HsnUSBPlugin::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar)
{
    auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
        registrar->messenger(), "hsn_usb_connection",
        &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<HsnUSBPlugin>(registrar);

    plugin->m_event_channel = std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
        registrar->messenger(), "USBEventChannel",
        &flutter::StandardMethodCodec::GetInstance());

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result)
        {
            plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    HsnUSBStreamHandler<> *_handler = new HsnUSBStreamHandler<>();
    plugin->m_handler = _handler;
    auto _stream_handle = static_cast<flutter::StreamHandler<flutter::EncodableValue> *>(plugin->m_handler);
    std::unique_ptr<flutter::StreamHandler<flutter::EncodableValue>> _ptr{_stream_handle};
    plugin->m_event_channel->SetStreamHandler(std::move(_ptr));

    registrar->AddPlugin(std::move(plugin));
}

HsnUSBPlugin::HsnUSBPlugin(flutter::PluginRegistrarWindows *registrar)
{
    this->registrar = registrar;
    window_proc_id = registrar->RegisterTopLevelWindowProcDelegate(
        [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
        {
            return HandleWindowProc(hwnd, message, wparam, lparam);
        });
}

HsnUSBPlugin::~HsnUSBPlugin()
{
    registrar->UnregisterTopLevelWindowProcDelegate(window_proc_id);
}

std::string HsnUSBPlugin::connectUSB(const int &result)
{
    // 여기서 USB 연결 시도.
    result_str += std::to_string(result);

    return result_str;
}

void HsnUSBPlugin::HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &method_call, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
{

    if (method_call.method_name().compare("hsn_usb_connection") == 0)
    {
        result->Success(flutter::EncodableValue(HsnUSBPlugin::connectUSB(2)));
        return;
    }

    result->NotImplemented();
}

std::optional<LRESULT> HsnUSBPlugin::HandleWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    std::optional<LRESULT> result = std::nullopt;
    if (message == WM_DEVICECHANGE)
    {
        switch (wparam)
        {
        case DBT_DEVICEARRIVAL:
            m_handler->onCallback(flutter::EncodableValue(HsnUSBPlugin::connectUSB(1)));
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            m_handler->onCallback(flutter::EncodableValue(HsnUSBPlugin::connectUSB(0)));
            break;
        }
    }
    return result;
}
