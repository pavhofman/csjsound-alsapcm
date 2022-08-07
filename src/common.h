#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <alsa/asoundlib.h>
#include <string.h>
#include <jni.h>

#include "config.h"
#include "types.h"
#include "debug.h"

// value used in java
#define NOT_SPECIFIED   -1

typedef struct {
	// used when walking descs to find a desc with required idx
	int down_counter;
    char deviceID[STR_LEN+1];
    INT32 maxLines;
    char name[STR_LEN+1];
    char vendor[STR_LEN+1];
    char description[STR_LEN+1];
} MixerDesc;

typedef struct {
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* hwParams;
    snd_pcm_sw_params_t* swParams;
    int bufferBytes;
    int frameBytes;
    unsigned int periods;
    snd_pcm_uframes_t periodSize;
    short int isRunning;
    short int isFlushed;
} PcmInfo;

typedef struct {
    JNIEnv *env;
    jobject vector;
    jclass clazz;
    jmethodID methodID;
} AddFmtMethodInfo;

// callback from impl to iface
void clbkAddAudioFmt(AddFmtMethodInfo* mInfo, int sampleSignBits, int frameBytes,
		int channels, int rate, int enc, int isSigned, int bigEndian);

INT32 doGetMixerCnt();
INT32 doFillDesc(MixerDesc* description);
void doGetFmts(const char* deviceID, int isSource, AddFmtMethodInfo* mInfo);
PcmInfo* doOpen(const char* deviceID, int isSource, int enc, int rate, int sampleSignBits,
		int frameBytes, int channels, int isSigned, int isBigEndian, int bufferBytes);
void doClose(PcmInfo* info, int isSource);
int doStart(PcmInfo* info, int isSource);
int doStop(PcmInfo* info, int isSource);
int doRead(PcmInfo* info, char* buffer, int bytes);
int doWrite(PcmInfo* info, char* buffer, int bytes);
void doDrain(PcmInfo* info);
void doFlush(PcmInfo* info, int isSource);
int doGetAvailBytes(PcmInfo* info, int isSource);
INT64 doGetBytePos(PcmInfo* info, int isSource, INT64 javaBytePos);

#endif // COMMON_INCLUDED
