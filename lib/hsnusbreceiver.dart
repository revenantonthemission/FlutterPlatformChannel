import 'package:flutter/services.dart';

class HsnUSBReceiver {
  static const _eventChannel = EventChannel('USBEventChannel');

  static Stream<String> streamReceiver() {
    return _eventChannel
        .receiveBroadcastStream()
        .map((event) => event.toString())
        .distinct();
  }
}
