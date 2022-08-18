#include <malloc.h>

#include "common.h"
// generated JNI headers
#include "com_cleansine_sound_provider_SimpleMixer.h"
#include "com_cleansine_sound_provider_SimpleMixerProvider.h"

#define ADD_FORMAT_METHOD   "addFormat"

// called from impl
void clbkAddAudioFmt(AddFmtMethodInfo* mInfo, int sampleSignBits, int frameBytes,
	int channels, int rate, int enc, int isSigned, int bigEndian)
{
    if (frameBytes <= 0) {
        if (channels > 0) {
            frameBytes = ((sampleSignBits + 7) / 8) * channels;
        } else {
            frameBytes = -1;
        }
    }
    TRACE4("%s: sampleSignBits=%d, frameBytes=%d, channels=%d, ",
           __FUNCTION__, sampleSignBits, frameBytes, channels);
    TRACE4(" rate=%d, enc=%d, signed=%d, bigEndian=%d\n",
           (int) rate, enc, isSigned, bigEndian);
    (*mInfo->env)->CallStaticVoidMethod(mInfo->env, mInfo->clazz, mInfo->methodID,
            mInfo->vector, sampleSignBits, frameBytes, channels, rate, enc, isSigned, bigEndian);
}

JNIEXPORT void JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nGetFormats
	(JNIEnv *env, jclass clazz, jstring deviceID, jboolean isSource, jobject formats)
{
    AddFmtMethodInfo mInfo;

    mInfo.env = env;
    mInfo.vector = formats;
    mInfo.clazz = clazz;
    mInfo.methodID = (*env)->GetStaticMethodID(env, clazz, ADD_FORMAT_METHOD, "(Ljava/util/Vector;IIIIIZZ)V");
    if (mInfo.methodID == NULL) {
        ERROR1("Could not get method ID for %s!\n", ADD_FORMAT_METHOD);
    } else {
        const char *utf_deviceID = (*env)->GetStringUTFChars(env, deviceID, 0);
        doGetFmts(utf_deviceID, (int) isSource, &mInfo);
        (*env)->ReleaseStringUTFChars(env, deviceID, utf_deviceID);
    }
}

JNIEXPORT jlong JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nOpen
	(JNIEnv* env, jclass clazz, jstring deviceID, jboolean isSource,
	jint enc, jint rate, jint sampleSignBits, jint frameBytes, jint channels,
	jboolean isSigned, jboolean isBigEndian, jint bufferBytes)
{
    const char *utf_deviceID = (*env)->GetStringUTFChars(env, deviceID, 0);
    PcmInfo* info =doOpen(utf_deviceID, (int) isSource,
                             (int) enc, (int) rate, (int) sampleSignBits,
                             (int) frameBytes, (int) channels,
                             (int) isSigned, (int) isBigEndian, (int) bufferBytes);
    (*env)->ReleaseStringUTFChars(env, deviceID, utf_deviceID);
    return (jlong) (UINT_PTR) info;
}

JNIEXPORT void JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nStart
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    if (info) {
        doStart(info, (int) isSource);
    }
}

JNIEXPORT void JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nStop
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    if (info) {
        doStop(info, (int) isSource);
    }
}


JNIEXPORT void JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nClose
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    if (info) {
        doClose(info, (int) isSource);
        free(info);
    }
}

JNIEXPORT jint JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nWrite
	(JNIEnv *env, jclass clazz, jlong nativePtr, jbyteArray jData, jint offset, jint len)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    int ret = -1;
    if (offset < 0 || len < 0) {
        ERROR2("nWrite: wrong parameters: offset=%d, len=%d\n", offset, len);
        return ret;
    }
    if (len == 0) {
        return 0;
    }
    if (info) {
        jboolean didCopy;
        UINT8* data = (UINT8*) ((*env)->GetByteArrayElements(env, jData, &didCopy));
        if (data == NULL)
            return ret;
        UINT8* offsetData = data;
        offsetData += (int) offset;
        ret = doWrite(info, (INT8*) offsetData, (int) len);
        (*env)->ReleaseByteArrayElements(env, jData, (jbyte*) data, JNI_ABORT);
    }
    return (jint) ret;
}

JNIEXPORT jint JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nRead
	(JNIEnv* env, jclass clazz, jlong nativePtr, jbyteArray jData, jint offset, jint len)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    int ret = -1;
    if (offset < 0 || len < 0) {
        ERROR3("%s: wrong parameters: offset=%d, len=%d\n", __FUNCTION__, offset, len);
        return ret;
    }
    if (info) {
        char* data = (char*) ((*env)->GetByteArrayElements(env, jData, NULL));
        if (data == NULL)
            return ret;
        char* offsetData = data;
        offsetData += (int) offset;
        ret = doRead(info, offsetData, (int) len);
        (*env)->ReleaseByteArrayElements(env, jData, (jbyte*) data, 0);
    }
    return (jint) ret;
}

JNIEXPORT jint JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nGetBufferBytes
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    int ret = -1;
    if (info) {
        ret = info->bufferBytes;
    }
    return (jint) ret;
}

JNIEXPORT void JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nDrain
	(JNIEnv* env, jclass clazz, jlong nativePtr)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    if (info) {
        doDrain(info);
    }
}


JNIEXPORT void JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nFlush
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;

    if (info) {
        doFlush(info, (int) isSource);
    }
}


JNIEXPORT jint JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nGetAvailBytes
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    int ret = -1;
    if (info) {
        ret = doGetAvailBytes(info, (int) isSource);
    }
    return (jint) ret;
}


JNIEXPORT jlong JNICALL Java_com_cleansine_sound_provider_SimpleMixer_nGetBytePos
	(JNIEnv* env, jclass clazz, jlong nativePtr, jboolean isSource, jlong javaBytePos)
{
    PcmInfo* info = (PcmInfo*) (UINT_PTR) nativePtr;
    INT64 ret = (INT64) javaBytePos;
    if (info) {
        ret = doGetBytePos(info, (int) isSource, (INT64) javaBytePos);
    }
    return (jlong) ret;
}

JNIEXPORT jint JNICALL Java_com_cleansine_sound_provider_SimpleMixerProvider_nGetMixerCnt
	(JNIEnv *env, jclass clazz)
{
    INT32 mixerCnt = 0;
    TRACE1("%s: starting\n", __FUNCTION__);
    mixerCnt = doGetMixerCnt();
    TRACE2("%s: mixerCnt %d\n", __FUNCTION__, mixerCnt);
    return (jint)mixerCnt;
}

JNIEXPORT jobject JNICALL Java_com_cleansine_sound_provider_SimpleMixerProvider_nCreateMixerInfo
	(JNIEnv *env, jclass clazz, jint idx)
{
    jmethodID constr;
    MixerDesc desc;
    jobject mixerInfo = NULL;
    jstring name;
    jstring deviceID;
    jstring vendor;
    jstring description;

    TRACE2("%s: idx %d\n", __FUNCTION__, idx);

    jclass infoCls = (*env)->FindClass(env, "com/cleansine/sound/provider/SimpleMixerInfo");
    if (infoCls == NULL) {
        ERROR1("%s: infoCls is NULL\n", __FUNCTION__);
        return NULL;
    }
    constr = (*env)->GetMethodID(env, infoCls, "<init>",
                                 "(ILjava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (constr == NULL) {
        ERROR1("%s: constr is NULL\n", __FUNCTION__);
        return NULL;
    }

    desc.down_counter = idx;
    strcpy(desc.deviceID, "UNKNOWN");
    desc.maxLines = 0;
    strcpy(desc.name, "UNKNOWN");
    strcpy(desc.vendor, "UNKNOWN");
    strcpy(desc.description, "UNKNOWN");

    if (doFillDesc(&desc)) {
        name = (*env)->NewStringUTF(env, desc.name);
        if (name == NULL)
            return mixerInfo;
        deviceID = (*env)->NewStringUTF(env, desc.deviceID);
        if (deviceID == NULL)
            return mixerInfo;
        vendor = (*env)->NewStringUTF(env, desc.vendor);
        if (vendor == NULL)
            return mixerInfo;
        description = (*env)->NewStringUTF(env, desc.description);
        if (description == NULL)
            return mixerInfo;
        mixerInfo = (*env)->NewObject(env, infoCls, constr, idx, deviceID,
                                      desc.maxLines, name, vendor, description);
    } else {
        ERROR1("%s: doFillDesc(desc) returned FALSE!\n", __FUNCTION__);
    }

    TRACE1("%s done.\n", __FUNCTION__);
    return mixerInfo;
}


JNIEXPORT jboolean JNICALL Java_com_cleansine_sound_provider_SimpleMixerProvider_nInit
  (JNIEnv *env, jclass clazz, jint logLevelID, jstring logTarget)
{
	// logging is compiled-in, log params are ignored
	return (jboolean) 1;
}
