#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <cerrno>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"FFMPEG_AV_PRO",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"FFMPEG_AV_PRO",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FFMPEG_AV_PRO",__VA_ARGS__)

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/jni.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
}

static char *audio_file_path = nullptr;

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGD("JNI_OnLoad: ");
    av_jni_set_java_vm(vm, nullptr);
    return JNI_VERSION_1_6;
}

inline double r2d(AVRational avRational) {
    return avRational.num == 0 || avRational.den == 0 ? 0.0 : (double) avRational.num /
                                                              (double) avRational.den;
}

inline long long getNowMs() {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    int seconds = tv.tv_sec % 360000;
    long long nowMs = seconds * 1000 + tv.tv_usec / 1000;
    return nowMs;
}

/**
 * 创建slengine
 * @return
 */
SLEngineItf createSLEngine() {
    SLresult re = SL_RESULT_SUCCESS;
    SLObjectItf slEngineObj = nullptr;
    re = slCreateEngine(&slEngineObj, 0, nullptr, 0, nullptr, nullptr);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("slCreateEngine failed!");
        return nullptr;
    }
    re = (*slEngineObj)->Realize(slEngineObj, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("slEngineObj Realize failed!");
        return nullptr;
    }
    SLEngineItf slEngine = nullptr;
    re = (*slEngineObj)->GetInterface(slEngineObj, SL_IID_ENGINE, &slEngine);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("slEngineObj GetInterface failed!");
        return nullptr;
    }

    return slEngine;
}

/**
 * 音频播放器器消费完pcm数据的回调
 * 不断调用SLAndroidSimpleBufferQueueItf.Enqueue()赛入数据消费，就会不断回到这个方法，从而形成连续播放音频数据
 * @param caller
 * @param pContext
 */
void pcmCallBack(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    LOGD("pcmCallBack");
    static FILE *file = nullptr;
    static char *buffer = nullptr;
    if (!buffer) {
        buffer = new char[1024 * 1024];
    }
    if (!file) {
        LOGD("pcmCallBack: audio_file_path: %s", audio_file_path);
        // todo Android11 以上无法打开sdcard上数据
        file = fopen(audio_file_path, "rb");
    }
    if (!file) {
        LOGE("file is null! errno = %d, reason = %s", errno, strerror(errno));
        return;
    }
    if (feof(file) == 0) {
        int len = fread(buffer, 1, 1024, file);
        if (len > 0) {
            (*caller)->Enqueue(caller, buffer, len);
        }
    }
}

extern "C"
JNIEXPORT jstring
JNICALL
Java_aplay_testffmpeg_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, nullptr);
    std::string hello = "Hello from C++ \n";
    hello += avcodec_configuration();
    env->ReleaseStringUTFChars(url_, url);
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_aplay_testffmpeg_SimplePlayer_nativeOpenVideo(JNIEnv *env, jobject thiz, jstring url_, jint w, jint h, jobject surface) {
    const char *url = env->GetStringUTFChars(url_, nullptr);
    const int surface_w = w;
    const int surface_h = h;
    int video_w = 0;
    int video_h = 0;
    //初始化解封装
    av_register_all();
    //初始化网络
    avformat_network_init();
    avcodec_register_all();

    /**
     * 1.打开文件
     * 此处不能直接定义**ic对象，然后再将它指定给avformat_open_input()函数
     */
    AVFormatContext *ic = nullptr;
    int re = avformat_open_input(&ic, url, nullptr, nullptr);
    if (re != 0) {
        LOGE("avformat_open_input failed!:%s", av_err2str(re));
        return;
    }
    LOGD("avformat_open_input %s success! surface_w = %d, surface_h = %d", url, surface_w, surface_h);

    // 2.获取流信息(包括音频流和视频流)
    re = avformat_find_stream_info(ic, nullptr);
    if (re != 0) {
        LOGE("avformat_find_stream_info failed!:%s", av_err2str(re));
        return;
    }
    LOGD("duration = %lld, nb_streams = %d", ic->duration, ic->nb_streams);
    int fps;
    int videoIndex;
    int audioIndex;
    for (int i = 0; i < ic->nb_streams; ++i) {
        AVStream *as = ic->streams[i];
        if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGD("find video stream.");
            videoIndex = i;
            fps = r2d(as->avg_frame_rate);
            LOGD("videoIndex = %d, fps = %d, width = %d, height = %d, codeid = %d, pixformat = %d",
                 videoIndex, fps, as->codecpar->width, as->codecpar->height, as->codecpar->codec_id,
                 as->codecpar->format);
        } else if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGD("find audio stream.");
            audioIndex = i;
            LOGD("audioIndex = %d, sample_rate = %d, channels = %d, sample_format = %d", audioIndex,
                 as->codecpar->sample_rate, as->codecpar->channels, as->codecpar->format);
        }
    }
    /**
     * 视频解封装并找到解码器
     */
    AVCodec *videoCodec = nullptr;
    videoIndex = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);
    // 硬件解码
    //videoCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    /**
     * 使用获取到的视频解码器
     * 再创建并初始化解码器上下文
     * 然后打开解码器
     */
    AVCodecContext *vc = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(vc, ic->streams[videoIndex]->codecpar);
    vc->thread_count = 4;
    avcodec_open2(vc, videoCodec, nullptr);
    video_w = vc->width;
    video_h = vc->height;
    LOGD("videoIndex = %d, videoCodec(%p)[name: %s], video[width = %d, height = %d]", videoIndex,
         videoCodec, videoCodec->name, video_w, video_h);
    /**
     * 音频解封装并找到解码器
     */
    AVCodec *audioCodec = nullptr;
    audioIndex = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);
    /**
     * 使用获取到的音频解码器
     * 再创建并初始化解码器上下文
     * 然后打开解码器
     */
    AVCodecContext *ac = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(ac, ic->streams[audioIndex]->codecpar);
    ac->thread_count = 1;
    avcodec_open2(ac, audioCodec, nullptr);
    LOGD("audioIndex = %d, audioCodec(%p)[name]: %s", audioIndex, audioCodec, audioCodec->name);

    long long start = getNowMs();
    int frameCount = 0;
    /**
     * 解码音视频
     * 先获取packet->发送packet->解码接收frame
     * 发送一次packet，可能需要获取几次frame
     */

    AVFrame *frame = av_frame_alloc();
    AVPacket *pkt = av_packet_alloc();

    // 视频图像转换
    SwsContext *vctx = nullptr;
    /**
     * todo
     * 此处使用surface的尺寸会有异常
     * 暂时还不太清楚他们的计算关系
     */
    int outW = 1280;
    int outH = 720;
    //存放由yuv转换而来的rgba图像数据
    AVFrame *rgba_dst_frame = av_frame_alloc();
    rgba_dst_frame->format = AV_PIX_FMT_RGBA;
    rgba_dst_frame->width = outW;
    rgba_dst_frame->height = outH;
    av_frame_get_buffer(rgba_dst_frame, 4 * 8/*RGBA四通道，每个通道8位*/);
    int rgba_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, rgba_dst_frame->width, rgba_dst_frame->height, 4 * 8/*RGBA四通道，每个通道8位*/);

    // 音频重采样
    SwrContext *actx = swr_alloc();
    actx = swr_alloc_set_opts(actx,
                              av_get_default_channel_layout(ac->channels), AV_SAMPLE_FMT_S16, ac->sample_rate,
                              av_get_default_channel_layout(ac->channels), ac->sample_fmt,
                              ac->sample_rate,
                              0, nullptr);
    re = swr_init(actx);
    if (re != 0) {
        LOGE("swr_init failed!");
    }
    char *pcm = new char[48000 * 4 * 2];

    //Android窗口相关
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_setBuffersGeometry(win, outW, outH, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;

    AVCodecContext *cc;
    int readPacketResult = av_read_frame(ic, pkt);
    //接收frame
    while (readPacketResult == 0) {
        //LOGD("pkt(%p)[pts: %lld, dts: %lld, size: %d, stream_index: %d]", pkt, pkt->pts, pkt->dts, pkt->size, pkt->stream_index);
        if (pkt->stream_index == audioIndex) {
            //LOGD("ready to decode audio data.");
            cc = ac;
        } else {
            //LOGD("ready to decode video data.");
            cc = vc;
        }
        // 发送packet
        re = avcodec_send_packet(cc, pkt);
        if (re != 0) {
            LOGE("avcodec_send_packet failed!");
            continue;
        }
        av_packet_unref(pkt);
        //接收frame
        int receiveFrameResult = avcodec_receive_frame(cc, frame);
        while (receiveFrameResult == 0) {
            if (cc == vc) {
                //LOGD("frame(%p)[pts: %lld, dts: %lld, size: %d]", frame, frame->pts, frame->pkt_dts, frame->pkt_size);
                frameCount++;
                vctx = sws_getCachedContext(vctx,
                                            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                                            rgba_dst_frame->width, rgba_dst_frame->height, (AVPixelFormat) rgba_dst_frame->format,
                                            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
                if (!vctx) {
                    LOGE("sws_getCachedContext failed!");
                } else {
                    // 视频图像格式转换
                    int scale_h = sws_scale(vctx,
                                            frame->data, frame->linesize, 0, frame->height,
                                            rgba_dst_frame->data, rgba_dst_frame->linesize);
                    //LOGD("sws_scale = %d", h);
                    if (scale_h > 0) {
                        // 视频数据上屏
                        ANativeWindow_lock(win, &window_buffer, nullptr);
                        auto *win_dst = static_cast<uint8_t *>(window_buffer.bits);
                        memcpy(win_dst, rgba_dst_frame->data[0], rgba_buffer_size);
                        ANativeWindow_unlockAndPost(win);
                    }
                }
            } else {
                uint8_t *out[2] = {nullptr};
                out[0] = reinterpret_cast<uint8_t *>(pcm);
                // 音频重采样
                int len = swr_convert(actx,
                                      out, frame->nb_samples,
                                      (const uint8_t **) frame->data, frame->nb_samples);
                //LOGD("swr_convert = %d", len);
            }
            if (getNowMs() - start >= 1000) {
                LOGD("decode video frame %d/s", frameCount);
                frameCount = 0;
                start = getNowMs();
            }
            receiveFrameResult = avcodec_receive_frame(cc, frame);
        }
        readPacketResult = av_read_frame(ic, pkt);
    }
    LOGW("======do finish======");
    // 释放内存
    delete[]pcm;
    sws_freeContext(vctx);
    swr_free(&actx);
    av_frame_free(&frame);
    av_frame_free(&rgba_dst_frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ac);
    avcodec_free_context(&vc);
    avformat_close_input(&ic);
    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_aplay_testffmpeg_SimplePlayer_nativeOpenAudio(JNIEnv *env, jobject thiz, jstring url_) {
    audio_file_path = const_cast<char *>(env->GetStringUTFChars(url_, nullptr));
    LOGD("audio url = %s", audio_file_path);

    /**
     * 1.创建引擎slengine
     */
    SLEngineItf slEngine = createSLEngine();
    if (!slEngine) {
        LOGE("create engine failed!");
        return;
    }

    SLresult re = SL_RESULT_SUCCESS;
    /**
     * 2.使用引擎创建混音器，并初始化
     */
    SLObjectItf slMixObj = nullptr;
    re = (*slEngine)->CreateOutputMix(slEngine, &slMixObj, 0, nullptr, nullptr);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("CreateOutputMix failed!");
        return;
    }
    re = (*slMixObj)->Realize(slMixObj, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("Mix Realize failed!");
        return;
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, slMixObj};
    SLDataSink audioSink = {&outputMix, nullptr};

    /**
     * 3.配置音频信息
     */
    // Android采样缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue audioBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    // 音频格式
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM,
                                  2,
                                  SL_SAMPLINGRATE_44_1,
                                  SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                  SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSource = {&audioBufferQueue, &formatPcm};

    /**
     * 4.创建音频播放器
     */
    const SLInterfaceID interfaceIds[] = {SL_IID_BUFFERQUEUE};
    const SLboolean interfaceReq[] = {SL_BOOLEAN_TRUE};
    SLObjectItf audioPlayerObj = nullptr;
    re = (*slEngine)->CreateAudioPlayer(slEngine, &audioPlayerObj, &audioSource, &audioSink, sizeof(interfaceIds) / sizeof(SLInterfaceID), interfaceIds,
                                        interfaceReq);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("CreateAudioPlayer failed!");
        return;
    }

    re = (*audioPlayerObj)->Realize(audioPlayerObj, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer Realize failed!");
        return;
    }

    /**
     * 获取音频播放器接口PLAY
     */
    SLPlayItf audioPlayerInterface = nullptr;
    re = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_PLAY, &audioPlayerInterface);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer GetInterface SL_IID_PLAY failed!");
        return;
    }

    /**
    * 获取音频播放器接口BUFFERQUEUE
     *并注册回调函数
    */
    SLAndroidSimpleBufferQueueItf pcmBufferQueueInterface = nullptr;
    re = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_BUFFERQUEUE, &pcmBufferQueueInterface);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("AudioPlayer GetInterface SL_IID_BUFFERQUEUE failed!");
        return;
    }
    re = (*pcmBufferQueueInterface)->RegisterCallback(pcmBufferQueueInterface, pcmCallBack, nullptr);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("buffer queue RegisterCallback failed!");
        return;
    }

    //改变状态机状态为播放PLAYING
    re = (*audioPlayerInterface)->SetPlayState(audioPlayerInterface, SL_PLAYSTATE_PLAYING);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("SetPlayState failed!");
        return;
    }

    //启动队列回调
    re = (*pcmBufferQueueInterface)->Enqueue(pcmBufferQueueInterface, "", 1);
    if (re != SL_RESULT_SUCCESS) {
        LOGE("buffer queue Enqueue buffer failed!");
        return;
    }
}