#include <jni.h>
#include <string>
#include <cstdint>
#include <cstdlib>
#include "opus.h"
#include "opus_types.h"
#include <android/log.h>


#define LOG_TAG "OPUS_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


extern "C"
{
JNIEXPORT jlong

JNICALL
Java_com_book_studio_opus_OpusBridge_createEncoder(JNIEnv *env, jobject thiz, jint sampleRateInHz,
                                                   jint channelConfig, jint complexity) {
    int error;
    LOGE("create Opus encoder; rate=%d channel=%d", sampleRateInHz, channelConfig);
    OpusEncoder *pOpusEnc = opus_encoder_create(sampleRateInHz, channelConfig,
                                                OPUS_APPLICATION_RESTRICTED_LOWDELAY,
                                                &error);
    if (error != OPUS_OK) {
        LOGE("Cannot create encoder: %s\n", opus_strerror(error));
        return 0;
    }
    if (pOpusEnc) {
        int bit_depth = 16;
        opus_encoder_ctl(pOpusEnc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));                   // 信号类型，VOICE语音，MUSIC音乐
        opus_encoder_ctl(pOpusEnc, OPUS_SET_BITRATE(sampleRateInHz));                     // 控制最大比特率，AUTO在不说话时减少带宽。
        opus_encoder_ctl(pOpusEnc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));          // 自动带宽，根据信号自动调整带宽
        opus_encoder_ctl(pOpusEnc, OPUS_SET_VBR(1));                                      // 0固定码率，1动态码率
        opus_encoder_ctl(pOpusEnc, OPUS_SET_VBR_CONSTRAINT(0));                           // 0不受约束，1受约束（默认）
        opus_encoder_ctl(pOpusEnc, OPUS_SET_COMPLEXITY(complexity));                      // 编码复杂度0~10
        opus_encoder_ctl(pOpusEnc, OPUS_SET_INBAND_FEC(0));                               // 算法修复丢失的数据包，0关，1开
        opus_encoder_ctl(pOpusEnc, OPUS_SET_FORCE_CHANNELS(channelConfig));               // 声道数
        opus_encoder_ctl(pOpusEnc, OPUS_SET_DTX(0));                                      // 不连续传输，0关，1开
        opus_encoder_ctl(pOpusEnc, OPUS_SET_LSB_DEPTH(bit_depth));                        // 位深
        opus_encoder_ctl(pOpusEnc, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_20_MS)); // 帧持续时间
        opus_encoder_ctl(pOpusEnc, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));          // 同编码器创建参数
    } else {
        LOGE("create Opus encoder error; error=%s", opus_strerror(error));
    }
    return (jlong)
            pOpusEnc;
}

JNIEXPORT jlong
JNICALL Java_com_book_studio_opus_OpusBridge_createDecoder
        (JNIEnv *env, jobject thiz, jint sampleRateInHz, jint channelConfig) {
    int error;
    OpusDecoder *pOpusDec = opus_decoder_create(sampleRateInHz, channelConfig, &error);
    return (jlong)
            pOpusDec;
}

JNIEXPORT jint

JNICALL Java_com_book_studio_opus_OpusBridge_encode
        (JNIEnv *env, jobject thiz, jlong pOpusEnc, jshortArray samples, jint offset,
         jbyteArray bytes) {
    OpusEncoder *pEnc = (OpusEncoder *) pOpusEnc;
    if (!pEnc || !samples || !bytes) {
        LOGE("encoder error");
        return 0;
    }
    jshort *pSamples = env->GetShortArrayElements(samples, 0);
    jsize nSampleSize = env->GetArrayLength(samples);
    jbyte *pBytes = env->GetByteArrayElements(bytes, 0);
    jsize nByteSize = env->GetArrayLength(bytes);
    if (nSampleSize - offset < 320 || nByteSize <= 0) {
        LOGE("encoder error");
        return 0;
    }
    int nRet = opus_encode(pEnc, pSamples + offset, nSampleSize, (unsigned char *) pBytes,
                           nByteSize);
    env->ReleaseShortArrayElements(samples, pSamples, 0);
    env->ReleaseByteArrayElements(bytes, pBytes, 0);
    return nRet;
}

JNIEXPORT jint

JNICALL Java_com_book_studio_opus_OpusBridge_decode
        (JNIEnv *env, jobject thiz, jlong pOpusDec, jbyteArray bytes, jshortArray samples) {
    OpusDecoder *pDec = (OpusDecoder *) pOpusDec;
    if (!pDec || !samples || !bytes) {
        LOGE("decoder error");
        return 0;
    }
    jshort *pSamples = env->GetShortArrayElements(samples, 0);
    jbyte *pBytes = env->GetByteArrayElements(bytes, 0);
    jsize nByteSize = env->GetArrayLength(bytes);
    jsize nShortSize = env->GetArrayLength(samples);
    if (nByteSize <= 0 || nShortSize <= 0) {
        LOGE("decoder error bytes.size = 0");
        return -1;
    }
    int nRet = opus_decode(pDec, (unsigned char *) pBytes, nByteSize, pSamples, nShortSize, 0);
    env->ReleaseShortArrayElements(samples, pSamples, 0);
    env->ReleaseByteArrayElements(bytes, pBytes, 0);
    return nRet;
}

JNIEXPORT void JNICALL
Java_com_book_studio_opus_OpusBridge_destroyEncoder(JNIEnv *env, jobject thiz, jlong pOpusEnc) {
    OpusEncoder *pEnc = (OpusEncoder *) pOpusEnc;
    if (!pEnc)
        return;
    opus_encoder_destroy(pEnc);
}

JNIEXPORT void JNICALL
Java_com_book_studio_opus_OpusBridge_destroyDecoder(JNIEnv *env, jobject thiz, jlong pOpusDec) {
    OpusDecoder *pDec = (OpusDecoder *) pOpusDec;
    if (!pDec)
        return;
    opus_decoder_destroy(pDec);
}

}
