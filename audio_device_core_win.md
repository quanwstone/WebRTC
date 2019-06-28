### 利用core audio进行音频采集

使用条件

Vista及以上.

#### 流程

创建IMMDeviceEnumerator对象.

    CoCreateInstance(
      __uuidof(MMDeviceEnumerator),
      NULL,
      CLSCTX_ALL,
      __uuidof(IMMDeviceEnumerator),
      reinterpret_cast<void**>(&_ptrEnumerator));
    assert(NULL != _ptrEnumerator);
生成一个音频终端设备的集合(采集或者预览).每个终端设备都支持IMMDevice和IMMEndpoint接口.

    hr = _ptrEnumerator->EnumAudioEndpoints(
                                 dataFlow,            // data-flow direction (input parameter)
                                 DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED | DEVICE_STATE_UNPLUGGED,
                                 &pCollection);     
通过IMMDeviceCollection对象获取内部的每个设备的音频控制信息.

       // Check the hardware volume capabilities.
        DWORD dwHwSupportMask = 0;
        hr = pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                               NULL, (void**)&pEndpointVolume);
### 数据采集

通过枚举器获取采集设备，利用默认的音频采集设备

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(
                                   dir,
                                   role,
                                   ppDevice);
通过输入设备获取对端音频控制器IAudioEndpointVolume

    ret = _ptrDeviceIn->Activate(__uuidof(IAudioEndpointVolume),
                                 CLSCTX_ALL,
                                 NULL,
                                 reinterpret_cast<void **>(&_ptrCaptureVolume));
创建输入设备IAudioClient接口,用于设置采集格式信息.

    hr = _ptrDeviceIn->Activate(
                          __uuidof(IAudioClient),
                          CLSCTX_ALL,
                          NULL,
                          (void**)&_ptrClientIn);
```
   // Set wave format
    Wfx.wFormatTag = WAVE_FORMAT_PCM;
    Wfx.wBitsPerSample = 16;
    Wfx.cbSize = 0;
```

    // Create a capturing stream.
    hr = _ptrClientIn->Initialize(
                          AUDCLNT_SHAREMODE_SHARED,             // share Audio Engine with other applications
                          AUDCLNT_STREAMFLAGS_EVENTCALLBACK |   // processing of the audio buffer by the client will be event driven
                          AUDCLNT_STREAMFLAGS_NOPERSIST,        // volume and mute settings for an audio session will not persist across system restarts
                          0,                                    // required for event-driven shared mode
                          0,                                    // periodicity
                          &Wfx,                                 // selected wave format
                          NULL);                                // session GUID
获取IAudioCaptureClient接口,用于获取采集数据

    hr = _ptrClientIn->GetService(
                          __uuidof(IAudioCaptureClient),
                          (void**)&_ptrCaptureClient);
启动采集线程

    // Start up the capturing stream.
    //
    hr = _ptrClientIn->Start();
       //  Find out how much capture data is available
            //
            hr = _ptrCaptureClient->GetBuffer(&pData,           // packet which is ready to be read by used
                                              &framesAvailable, // #frames in the captured packet (can be zero)
                                              &flags,           // support flags (check)
                                              &recPos,          // device position of first audio frame in data packet
                                              &recTime);        // value of performance counter at the time of recording the first audio frame
数据的同步是通过事件控制

    // Set the event handle that the system signals when an audio buffer is ready
    // to be processed by the client.
    hr = _ptrClientIn->SetEventHandle(
                          _hCaptureSamplesReadyEvent);


### 数据输出Render

//获取输出设备和音量控制器.

通过设备枚举器获取输出设备,因为输出设备多个，所以涉及到了枚举.

```
hr = _ptrEnumerator->EnumAudioEndpoints(
                           dir,
                           DEVICE_STATE_ACTIVE,        // only active endpoints are OK
                           &pCollection);
```

    hr = pCollection->Item(
                        index,
                        ppDevice);


获取设备控制器IAudioSessionManager

    IAudioSessionManager* pManager = NULL;
    ret = _ptrDeviceOut->Activate(__uuidof(IAudioSessionManager),
                                  CLSCTX_ALL,
                                  NULL,
                                  (void**)&pManager);
通过设备控制器获取设备音量控制接口

```
    ret = pManager->GetSimpleAudioVolume(NULL, FALSE, &_ptrRenderSimpleVolume);
```

通过输出设备创建IAudioClient对象.

    hr = _ptrDeviceOut->Activate(
                          __uuidof(IAudioClient),
                          CLSCTX_ALL,
                          NULL,
                          (void**)&_ptrClientOut);
通过IAudioClient设置音频格式信息

    hr = _ptrClientOut->Initialize(
                          AUDCLNT_SHAREMODE_SHARED,             // share Audio Engine with other applications
                          AUDCLNT_STREAMFLAGS_EVENTCALLBACK,    // processing of the audio buffer by the client will be event driven
                          hnsBufferDuration,                    // requested buffer capacity as a time value (in 100-nanosecond units)
                          0,                                    // periodicity
                          &Wfx,                                 // selected wave format
                          NULL);                                // session GUID
获取IAudioRenderClient，用于向终端设备写入播放数据.

    hr = _ptrClientOut->GetService(
                          __uuidof(IAudioRenderClient),
                          (void**)&_ptrRenderClient);
启动数据写入线程,内部启动开始数据输出,并向输出内存填充数据.

```
 hr = _ptrClientOut->Start();
```

通过找到合适的内存地址填充数据即可.

```
   hr = _ptrRenderClient->GetBuffer(_playBlockSize, &pData);
```

播出频率控制是通过填充特点时长的数据，当数据播放完后触发事件

    // Set the event handle that the system signals when an audio buffer is ready
    // to be processed by the client.
    hr = _ptrClientOut->SetEventHandle(
                          _hRenderSamplesReadyEvent);


### 总结

IAudioClient接口是用于在客户端和音频引擎之间创建一个音频流.通过IAudioClient创建出IAudioCaptureClient和IAudioRenderClient接口.采集的帧率控制是在初始化时信息设置，渲染的帧率控制在于初始化时缓存buffer的时长.