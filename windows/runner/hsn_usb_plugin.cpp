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

/*void LOG_D(const char *format, ...);
void notifyLoadingState(bool value);
void notifyProbeState(int32_t value);
void notifyErrorState(std::string err_str, int err_num);
void notifySelfTestPrgress(int progress);
void printError(int err_num);*/
// void HsnUSBPluginRegisterWithRegistrar(FlutterDesktopPluginRegistrarRef registrar);

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
    // 방법 1. USBReceiver.cpp의 함수들을 여기서 다시 구현
    // 방법 2. 실제 USB 연결은 USBReceiver 객체에게 맡기고, 여기서는 그 결과를 받는 방식.
    std::string result_str = "USB 연결 테스트 : ";

    /*USBReceiver usbReceiver(handleUSBEvent);
    auto handle = usbReceiver.getHandle();
    auto ret = HsnLibrary::initialize(handle);

    HsnLibrary::Callback::registerLoadingStateCallback(notifyLoadingState);
    HsnLibrary::Callback::registerProbeStateCallback(notifyProbeState);
    HsnLibrary::Callback::registerErrorStateCallback(notifyErrorState);
    HsnLibrary::Callback::registerSelfTestProgressCallback(notifySelfTestPrgress);


    HsnLibrary::Probe::detectConnection();
    HsnLibrary::Probe::connect(0);*/
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
/*
void LOG_D(const char *format, ...)
{
    static char buffer[8192]{0};
    va_list arg;
    va_start(arg, format);
    vsprintf(buffer, format, arg);
    va_end(arg);
    OutputDebugStringA(buffer);
    memset(buffer, 0, 8192);
}

void notifyLoadingState(bool value)
{
    printf("loading : %s\n", value ? "start" : "end");
}
void notifyProbeState(int32_t value)
{
    const char *obj_val = nullptr;
    switch (value)
    {
    case 0:
        obj_val = "DISABLE";
        break;
    case 1:
        obj_val = "ENABLE";
        break;
    case 2:
        obj_val = "CONNECTED";
        break;
    case 3:
        obj_val = "ACTIVATED";
        break;
    case 4:
        obj_val = "READY";
        break;
    case 5:
        obj_val = "STREAMING";
        break;
    }

    printf("Probe State : %s\n", obj_val);
    LOG_D("Probe State : %s\n", obj_val);
}
void notifyErrorState(std::string err_str, int err_num)
{
    printf("error : %s (%d)\n", err_str.c_str(), err_num);
}

void notifySelfTestPrgress(int progress)
{
    printf("progress : %d\n", progress);
}

void printError(int err_num)
{
    printf("error : %d\n", err_num);
}
*/