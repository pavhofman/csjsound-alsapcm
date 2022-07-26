#include "common.h"

static void alsaDbgOut(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
#ifdef USE_ERROR
    va_list args;
    va_start(args, fmt);
    printf("%s:%d function %s: error %d: %s\n", file, line, function, err, snd_strerror(err));
    if (strlen(fmt) > 0) {
        vprintf(fmt, args);
        printf("\n");
    }
    va_end(args);
#endif
}

static void alsaNODbgOut(const char *file, int line, const char *function, int err, const char *fmt, ...) {}

static int is_initialized = 0;

void initAlsalib()
{
    if (!is_initialized) {
        is_initialized = TRUE;
        snd_lib_error_set_handler(&alsaDbgOut);
    }
}

int openDeviceID(const char* deviceID, snd_pcm_t** handle, int isSource, int logError)
{
    initAlsalib();
    TRACE2("%s: Opening PCM device %s\n", __FUNCTION__, deviceID);
    if (!logError) {
        // disabling alsa debug
        snd_lib_error_set_handler(&alsaNODbgOut);
    }
    int ret = snd_pcm_open(handle, deviceID,
                       isSource? SND_PCM_STREAM_PLAYBACK: SND_PCM_STREAM_CAPTURE,
                       SND_PCM_NONBLOCK);
	// restoring alsa debug
	snd_lib_error_set_handler(&alsaDbgOut);
    if (ret != 0) {
        if (logError) {
            ERROR3("%s: snd_pcm_open of deviceID %s: %s\n", __FUNCTION__, deviceID, snd_strerror(ret));
        } else {
            // only trace
            TRACE3("%s: snd_pcm_open of deviceID %s: %s\n", __FUNCTION__, deviceID, snd_strerror(ret));
        }
        *handle = NULL;
    }
    return ret;
}

typedef int (*DescBuilder)(const char* deviceID, const char* type, MixerDesc* desc);

int buildDesc(const char* deviceID, const char* type, MixerDesc* desc)
{
    initAlsalib();
    if (desc->down_counter == 0) {
        // we found the device with correct index
        TRACE2("%s: Building desc for device %s\n", __FUNCTION__, deviceID);
        desc->maxLines = 1;
        strncpy(desc->name, "PCM: ", STR_LEN);
        strncat(desc->name, deviceID, STR_LEN - strlen(desc->name));
        strncpy(desc->deviceID, deviceID, STR_LEN);
        strncpy(desc->vendor, "ALSA", STR_LEN);
        strncpy(desc->description, "Config type: ", STR_LEN);
        strncat(desc->description, type, STR_LEN - strlen(desc->description));
        // found, break walk
        return TRUE;
    } else {
        // not found, continue walking
        desc->down_counter--;
        return FALSE;
    }
}


static int ignoreConfig(const char *configID)
{
    int i = 0;
    while(IGNORED_CONFIGS[i]) {
        if (!strcmp(configID, IGNORED_CONFIGS[i])) {
            return TRUE;
        }
        ++i;
    }
    return FALSE;
}

// returns count of configs walked or -1 if error
int walkConfigs(DescBuilder builder, MixerDesc* desc)
{
    snd_config_t *topNode = NULL;
    int cnt = 0;
    int ret;

    /* Iterate over configured PCM devices */
    if (NULL == snd_config) {
        TRACE1("%s: Updating snd_config\n", __FUNCTION__);
        ret = snd_config_update();
        if (ret < 0) {
            ERROR2("%s: snd_config_update: %s\n", __FUNCTION__, snd_strerror(ret));
            return -1;
        }
    }
    ret = snd_config_search(snd_config, "pcm", &topNode);
    if (ret >= 0) {
        snd_config_iterator_t i, next;
        snd_config_for_each(i, next, topNode) {
            const char *typeStr = "unknown";
            const char *idStr = NULL;

            snd_config_t *entry = snd_config_iterator_entry(i);
            snd_config_t * type = NULL;
            ret = snd_config_search(entry, "type", &type);
            if (ret < 0) {
                if (-ENOENT != ret) {
                    ERROR2("%s: snd_config_search: %s", __FUNCTION__, snd_strerror(ret));
                    return -1;
                }
            }
            else {
                ret = snd_config_get_string(type, &typeStr);
                if (ret < 0) {
                    ERROR2("%s: snd_config_get_string: %s", __FUNCTION__, snd_strerror(ret));
                    return -1;
                }
            }
            ret = snd_config_get_id(entry, &idStr);
            if (ret < 0) {
                ERROR2("%s: snd_config_get_id: %s", __FUNCTION__, snd_strerror(ret));
                return -1;
            }
            if (ignoreConfig(idStr)) {
                TRACE3("%s: Ignoring config [%s] of type [%s]\n", __FUNCTION__, idStr, typeStr);
                continue;
            }
            TRACE3("%s: Found config [%s] of type [%s]\n", __FUNCTION__, idStr, typeStr);
            ++cnt;
            if (builder != NULL) {
                // must build description
                if ((*builder)(idStr, typeStr, desc))
                    // found
                    break;
            }
        }
    } else
        ERROR2("%s: snd_config_search: %s\n", __FUNCTION__, snd_strerror(ret));
    return cnt;
}


INT32 doGetMixerCnt()
{
    return (INT32) walkConfigs(NULL, NULL);
}


INT32 doFillDesc(MixerDesc* desc)
{
    initAlsalib();
    TRACE2("%s: idx = %d\n", __FUNCTION__, desc->down_counter);
    int ret = walkConfigs(&buildDesc, desc);
    if (ret < 0)
        // error
        return FALSE;
    return (desc->down_counter == 0)? TRUE: FALSE;
}

void doGetFmts(const char* deviceID, int isSource, AddFmtMethodInfo* mInfo) {
    // opening the device to find out supported formats
    snd_pcm_t* handle;
    if (openDeviceID(deviceID, &handle, isSource, FALSE) < 0) {
        TRACE2("%s: opening device %s failed\n", __FUNCTION__, deviceID);
        return;
    }
    snd_pcm_format_mask_t* formatMask;
	snd_pcm_format_mask_alloca(&formatMask);
    snd_pcm_hw_params_t* hwParams;
    snd_pcm_hw_params_alloca(&hwParams);
    int ret = snd_pcm_hw_params_any(handle, hwParams);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_hw_params_any: %d\n", __FUNCTION__, ret);
        goto end;
    }

    unsigned int channelsMin, channelsMax;
    ret = snd_pcm_hw_params_get_channels_min(hwParams, &channelsMin);
    if (ret != 0) {
        ERROR2("%s: snd_pcm_hw_params_get_channels_min: %d\n", __FUNCTION__, ret);
        goto end;
    }
    ret = snd_pcm_hw_params_get_channels_max(hwParams, &channelsMax);
    if (ret != 0) {
        ERROR2("%s: snd_pcm_hw_params_get_channels_max: %d\n", __FUNCTION__, ret);
        goto end;
    }

    snd_pcm_hw_params_get_format_mask(hwParams, formatMask);
    int rate = -1;
    snd_pcm_format_t format;
    for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
        TRACE2("%s: testing format %s\n", __FUNCTION__, snd_pcm_format_name(format));
        if (!snd_pcm_format_mask_test(formatMask, format)) {
            TRACE2("%s: format mask does not fit format %s\n", __FUNCTION__, snd_pcm_format_name(format));
            continue;
        }

        // we support only PCM encoding
        if (snd_pcm_format_linear(format) < 1) {
            TRACE2("%s: skipping nonlinear format %s\n", __FUNCTION__, snd_pcm_format_name(format));
            continue;
        }
	    int sampleBytes = (snd_pcm_format_physical_width(format) + 7) / 8;
	    if (sampleBytes <= 0) {
            TRACE2("%s: snd_pcm_format_physical_width: nonpositive physical width for format %s\n",
                    __FUNCTION__, snd_pcm_format_name(format));
            continue;
	    }

	    int sampleSignBits = snd_pcm_format_width(format);
	    int enc = 0; // we support only PCM (=0)
	    int isSigned = (snd_pcm_format_signed(format) > 0);
	    int isBigEndian = (snd_pcm_format_big_endian(format) > 0);
        if (channelsMax - channelsMin > MAX_LISTED_CHANNELS) {
            // for large number of channels - only format with channelsMin, channelsMax, and unspecified
            clbkAddAudioFmt(mInfo, sampleSignBits, sampleBytes * channelsMin, channelsMin, rate, enc, isSigned, isBigEndian);
            clbkAddAudioFmt(mInfo, sampleSignBits, sampleBytes * channelsMax, channelsMax, rate, enc, isSigned, isBigEndian);
            // unspecified channels => unspecified frameBytes
            clbkAddAudioFmt(mInfo, sampleSignBits, -1, -1, rate, enc, isSigned, isBigEndian);
        } else {
            // individual format for all possible channel counts
            for (unsigned int channels = channelsMin; channels <= channelsMax; channels++) {
                clbkAddAudioFmt(mInfo, sampleSignBits, sampleBytes * channels, channels, rate, enc, isSigned, isBigEndian);
            }
        }
    }
  end:
    snd_pcm_close(handle);
}

int setDeviceStart(PcmInfo* info, int startAutomatically)
{
    int threshold;

    if (startAutomatically) {
        // anything written to the buffer makes the device start
        threshold = 1;
    } else {
        // never starting the device automatically - using large threshold
        threshold = 2000000000;
    }
    int ret = snd_pcm_sw_params_set_start_threshold(info->handle, info->swParams, threshold);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_sw_params_set_start_threshold: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    return TRUE;
}

int setDeviceStartAndCommit(PcmInfo* info, int startAutomatically) {
    int ret = 0;

    if (!setDeviceStart(info, startAutomatically)) {
        ret = -1;
    } else {
        ret = snd_pcm_sw_params(info->handle, info->swParams);
        if (ret < 0) {
            ERROR2("%s: Cannot set sw params: %s\n", __FUNCTION__, snd_strerror(ret));
        }
    }
    return (ret == 0)? TRUE: FALSE;
}

int setHWParams(PcmInfo* info, int rate, int channels, int bufferSize, snd_pcm_format_t format)
{
    int ret = snd_pcm_hw_params_any(info->handle, info->hwParams);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_hw_params_any: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }

    ret = snd_pcm_hw_params_set_access(info->handle, info->hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_hw_params_set_access: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }

    ret = snd_pcm_hw_params_set_format(info->handle, info->hwParams, format);
    if (ret < 0) {
        ERROR3("%s: snd_pcm_hw_params_set_format: format %s unavailable: %s\n", __FUNCTION__, snd_pcm_format_name(format), snd_strerror(ret));
        return FALSE;
    }

    ret = snd_pcm_hw_params_set_channels(info->handle, info->hwParams, channels);
    if (ret < 0) {
        ERROR3("%s: snd_pcm_hw_params_set_channels: channels cnt %d unavailable: %s\n", __FUNCTION__, channels, snd_strerror(ret));
        return FALSE;
    }

    int nearRate = rate;
    int ignDir = 0;
    ret = snd_pcm_hw_params_set_rate_near(info->handle, info->hwParams, &nearRate, &ignDir);
    if (ret < 0) {
        ERROR3("%s: snd_pcm_hw_params_set_rate_near: rate %d unavailable: %s\n", __FUNCTION__, rate, snd_strerror(ret));
        return FALSE;
    }
    if (abs(nearRate - rate) > 2) {
        ERROR3("%s: Rate does not match (req. %d, got %d)\n", __FUNCTION__, rate, nearRate);
        return FALSE;
    }

    snd_pcm_uframes_t finalBufferSize = (snd_pcm_uframes_t) bufferSize;
    ret = snd_pcm_hw_params_set_buffer_size_near(info->handle, info->hwParams, &finalBufferSize);
    if (ret < 0) {
        ERROR3("%s: snd_pcm_hw_params_set_buffer_size_near: cannot set buffer size to %d frames: %s\n", __FUNCTION__, (int) finalBufferSize, snd_strerror(ret));
        return FALSE;
    }
    bufferSize = (int) finalBufferSize;

    if (bufferSize > SMALL_BUFFER_SIZE_LIMIT) {
        ignDir = 0;
        unsigned int periodTime = DEFAULT_PERIOD_TIME;
        ret = snd_pcm_hw_params_set_period_time_near(info->handle, info->hwParams, &periodTime, &ignDir);
        if (ret < 0) {
            ERROR3("%s: snd_pcm_hw_params_set_period_time_near: cannot set period time to %d: %s\n", __FUNCTION__,DEFAULT_PERIOD_TIME, snd_strerror(ret));
            return FALSE;
        }
    } else {
        // small buffer, using only 2 periods per buffer for low latency
        ignDir = 0;
        unsigned int periods = 2;
        ret = snd_pcm_hw_params_set_periods_near(info->handle, info->hwParams, &periods, &ignDir);
        if (ret < 0) {
            ERROR3("%s: snd_pcm_hw_params_set_periods_near: cannot set period count to %d: %s\n", __FUNCTION__, periods, snd_strerror(ret));
            return FALSE;
        }
    }    
    ret = snd_pcm_hw_params(info->handle, info->hwParams);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_hw_params: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    return TRUE;
}

int setSWParams(PcmInfo* info) {
    int ret = snd_pcm_sw_params_current(info->handle, info->swParams);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_sw_params_current: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    if (!setDeviceStart(info, FALSE)) {
        return FALSE;
    }

    // at least periodSize must be available
    ret = snd_pcm_sw_params_set_avail_min(info->handle, info->swParams, info->periodSize);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_sw_params_set_avail_min: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    ret = snd_pcm_sw_params(info->handle, info->swParams);
    if (ret < 0) {
        ERROR2("%s: snd_pcm_sw_params: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    return TRUE;
}

/******** OPEN/CLOSE **********/
static snd_output_t* OUTPUT = NULL;

// returns either pointer or NULL
PcmInfo* doOpen(const char* deviceID, int isSource, int enc, int rate, int sampleBits,
                   int frameBytes, int channels, int isSigned, int isBigEndian, int bufferBytes)
{
	int ret;
    TRACE1("%s: start\n", __FUNCTION__);
#ifdef USE_TRACE
	// attaching output to stdio for debugging
    if (OUTPUT == NULL) {
        ret = snd_output_stdio_attach(&OUTPUT, stdout, 0);
        if (ret < 0) {
            ERROR2("%s: Output failed: %s\n",  __FUNCTION__, snd_strerror(ret));
            return NULL;
        }
    }
#endif
	// initial checks
    if (channels <= 0) {
        ERROR2("%s: Invalid number of channels=%d!\n", __FUNCTION__, channels);
        return NULL;
    }
    if (enc != 0) {
        ERROR2("%s: Only PCM encoding supported, not encoding %d!\n", __FUNCTION__, enc);
        return NULL;
    }
    int sampleBytes = frameBytes / channels;
    snd_pcm_format_t format = snd_pcm_build_linear_format(sampleBits, sampleBytes * 8,
            isSigned? 0: 1, isBigEndian? 1: 0);
    if (format == SND_PCM_FORMAT_UNKNOWN) {
        ERROR5("%s: Cannot find known ALSA enc for signBits %d, sampleBytes %d, signed %d, bigEndian %d!\n",
            __FUNCTION__, sampleBits, sampleBytes, isSigned, isBigEndian);
        return NULL;
    }

    PcmInfo* info = (PcmInfo*) malloc(sizeof(PcmInfo));
    if (!info) {
        ERROR1("%s: Out of memory\n", __FUNCTION__);
        return NULL;
    }
    memset(info, 0, sizeof(PcmInfo));
    info->isRunning = 0;
    info->isFlushed = 1;

    ret = openDeviceID(deviceID, &(info->handle), isSource, TRUE);
    if (ret == 0) {
        // starting with blocking mode
        snd_pcm_nonblock(info->handle, 0);
        ret = snd_pcm_hw_params_malloc(&(info->hwParams));
        if (ret != 0) {
            ERROR2("%s: snd_pcm_hw_params_malloc: %s\n", __FUNCTION__, snd_strerror(ret));
        } else {
            ret = -1;
            if (setHWParams(info, rate, channels, bufferBytes / frameBytes, format)) {
                // updating info from real HW params
				int ignDir = 0;
                info->frameBytes = frameBytes;
                ret = snd_pcm_hw_params_get_period_size(info->hwParams, &info->periodSize, &ignDir);
                if (ret < 0) {
                    ERROR2("%s: snd_pcm_hw_params_get_period: %s\n", __FUNCTION__, snd_strerror(ret));
                }
                snd_pcm_hw_params_get_periods(info->hwParams, &(info->periods), &ignDir);
                snd_pcm_uframes_t bufferSize = 0;
                snd_pcm_hw_params_get_buffer_size(info->hwParams, &bufferSize);
                info->bufferBytes = (int) bufferSize * frameBytes;
                TRACE4("%s: period size = %d, periods = %d. Buffer bytes: %d.\n",
                       __FUNCTION__, (int) info->periodSize, info->periods, info->bufferBytes);
            }
        }
        if (ret == 0) {
            ret = snd_pcm_sw_params_malloc(&(info->swParams));
            if (ret != 0) {
                ERROR2("%s: snd_pcm_hw_params_malloc: %s\n", __FUNCTION__, snd_strerror(ret));
            } else {
                if (!setSWParams(info)) {
                    ret = -1;
                }
            }
        }
        if (ret == 0) {
            ret = snd_pcm_prepare(info->handle);
            if (ret < 0) {
                ERROR2("%s: snd_pcm_prepare: %s\n", __FUNCTION__, snd_strerror(ret));
            }
        }

    }
    if (ret != 0) {
        doClose(info, isSource);
        info = NULL;
    } else {
        // all OK, setting to non-blocking mode
        snd_pcm_nonblock(info->handle, 1);
        TRACE3("%s: device %s opened OK for %s\n", __FUNCTION__, deviceID, isSource? "writing" : "reading");
    }
    return info;
}

void doClose(PcmInfo* info, int isSource)
{
    TRACE1("%s: start\n", __FUNCTION__);
    if (info != NULL) {
        if (info->handle != NULL) {
            snd_pcm_close(info->handle);
        }
        if (info->hwParams) {
            snd_pcm_hw_params_free(info->hwParams);
        }
        if (info->swParams) {
            snd_pcm_sw_params_free(info->swParams);
        }
    }
}

int doStart(PcmInfo* info, int isSource)
{
    int ret;
    TRACE1("%s: start\n", __FUNCTION__);
    snd_pcm_nonblock(info->handle, 0);
    // set start to autostart
    setDeviceStartAndCommit(info, TRUE);
    snd_pcm_state_t state = snd_pcm_state(info->handle);
    if (state == SND_PCM_STATE_PAUSED) {
        TRACE1("%s: Unpausing\n", __FUNCTION__);
        ret = snd_pcm_pause(info->handle, FALSE);
        if (ret != 0) {
            ERROR3("%s: snd_pcm_pause:%d: %s\n", __FUNCTION__, ret, snd_strerror(ret));
        }
    }
    if (state == SND_PCM_STATE_SUSPENDED) {
        TRACE1("%s: Resuming\n", __FUNCTION__);
        ret = snd_pcm_resume(info->handle);
        if (ret < 0) {
            if ((ret != -EAGAIN) && (ret != -ENOSYS)) {
                ERROR3("%s: snd_pcm_resume:%d: %s\n", __FUNCTION__, ret, snd_strerror(ret));
            }
        }
    }
    if (state == SND_PCM_STATE_SETUP) {
        TRACE1("%s: state SND_PCM_STATE_SETUP, must call prepare\n", __FUNCTION__);
        ret = snd_pcm_prepare(info->handle);
        if (ret < 0) {
            ERROR2("%s: snd_pcm_prepare: %s\n", __FUNCTION__, snd_strerror(ret));
        }
    }
    ret = snd_pcm_start(info->handle);
    if (ret != 0) {
        // EPIPE is a common error at start of the stream, do not log
        if (ret != -EPIPE) {
            ERROR3("%s: snd_pcm_start: %d: %s\n", __FUNCTION__, ret, snd_strerror(ret));
        }
    }
    ret = snd_pcm_nonblock(info->handle, 1);
    if (ret != 0) {
        ERROR2("%s: snd_pcm_nonblock: %s\n", __FUNCTION__, snd_strerror(ret));
    }
    state = snd_pcm_state(info->handle);
    TRACE2("%s: state %s\n", __FUNCTION__, snd_pcm_state_name(state));
    ret = (state == SND_PCM_STATE_PREPARED)
          || (state == SND_PCM_STATE_RUNNING)
          || (state == SND_PCM_STATE_XRUN)
          || (state == SND_PCM_STATE_SUSPENDED);
    if (ret) {
        info->isRunning = 1;
        if (!isSource) {
            info->isFlushed = 0;
        }
    }
    TRACE2("%s: %s\n", __FUNCTION__, ret? "OK": "failed");
    return ret? TRUE: FALSE;
}

int doStop(PcmInfo* info, int isSource)
{
    TRACE1("%s: start\n", __FUNCTION__);
    snd_pcm_nonblock(info->handle, 0);
    // preventing start after XRUN
    setDeviceStartAndCommit(info, FALSE);
    // pausing
    int ret = snd_pcm_pause(info->handle, 1);
    snd_pcm_nonblock(info->handle, 1);
    if (ret != 0) {
        ERROR2("%s: snd_pcm_pause: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    info->isRunning = 0;
    TRACE1("%s: success\n", __FUNCTION__);
    return TRUE;
}

/********** READ/WRITE *********/

// error recovery - decides among OK (1), try again (0), unrecoverable failure (-1)
int tryXRUNRecovery(PcmInfo* info, int err) {
    int ret;
    if (err == -EAGAIN) {
            TRACE1("%s: EAGAIN.\n", __FUNCTION__);
            // recoverable failure
            return 0;
    } else if (err == -EPIPE) {
        TRACE1("%s: XRUN.\n", __FUNCTION__);
        ret = snd_pcm_prepare(info->handle);
        if (ret < 0) {
            ERROR2("%s: Cannot recover from XRUN, snd_pcm_prepare: %s\n", __FUNCTION__, snd_strerror(ret));
            // unrecoverable failure
            return -1;
        }
        // recovered OK, will try read/write
        return 1;
    } else if (err == -ESTRPIPE) {
        TRACE1("%s: suspended.\n", __FUNCTION__);
        ret = snd_pcm_resume(info->handle);
        if (ret < 0) {
            if (ret == -EAGAIN) {
                // try again
                return 0;
            }
            // unrecoverable failure
            return -1;
        }
        ret = snd_pcm_prepare(info->handle);
        if (ret < 0) {
            ERROR2("%s: Cannot recover from XRUN in suspend, snd_pcm_prepare: %s\n", __FUNCTION__, snd_strerror(ret));
            // unrecoverable failure
            return -1;
        }
        return 1;
    }
    TRACE3("%s: unrecoverable error %d: %s\n", __FUNCTION__, err, snd_strerror(err));
    // got here, unrecoverable
    return -1;
}

int doRead(PcmInfo* info, char* buffer, int bytes) {
    int ret;
    TRACE2("%s: %d bytes\n", __FUNCTION__, bytes);
    if (bytes <= 0 || info->frameBytes <= 0) {
        ERROR3("%s: wrong bytes=%d, frameBytes=%d\n", __FUNCTION__, (int) bytes, (int) info->frameBytes);
        return -1;
    }
    if (!info->isRunning && info->isFlushed) {
        return 0;
    }
    int try = 0;
    snd_pcm_sframes_t framesToRead = (snd_pcm_sframes_t) (bytes / info->frameBytes);
    snd_pcm_sframes_t readFrames;
    do {
        readFrames = snd_pcm_readi(info->handle, buffer, framesToRead);
        if (readFrames < 0) {
            ret = tryXRUNRecovery(info, (int) readFrames);
            if (ret <= 0) {
                TRACE2("%s: tryXRUNRecovery: %d, returning.\n", __FUNCTION__, ret);
                return ret;
            }
            if (try++ > TRIES_TO_RECOVER) {
                ERROR2("%s: exceeded max tries %d to recover from xrun\n", __FUNCTION__, TRIES_TO_RECOVER);
                return -1;
            }
        } else {
            break;
        }
    } while (TRUE);
    ret =  (int) (readFrames * info->frameBytes);
    TRACE2("%s: read %d bytes.\n", __FUNCTION__, ret);
    return ret;
}

int doWrite(PcmInfo* info, char* buffer, int bytes) {
    int ret;
    TRACE2("%s: %d bytes\n", __FUNCTION__, bytes);
    if (bytes <= 0 || info->frameBytes <= 0) {
		ERROR3("%s: wrong bytes=%d, frameBytes=%d\n", __FUNCTION__, (int) bytes, (int) info->frameBytes);
        return -1;
    }
    int try = 0;
    snd_pcm_sframes_t framesToWrite = (snd_pcm_sframes_t) (bytes / info->frameBytes);
    snd_pcm_sframes_t writtenFrames;
    do {
        writtenFrames = snd_pcm_writei(info->handle, buffer, framesToWrite);
        if (writtenFrames < 0) {
            ret = tryXRUNRecovery(info, (int) writtenFrames);
            if (ret <= 0) {
                TRACE2("%s: tryXRUNRecovery: %d, returning.\n", __FUNCTION__, ret);
                return ret;
            }
            if (try++ > TRIES_TO_RECOVER) {
                ERROR2("%s: exceeded max tries %d to recover from xrun\n", __FUNCTION__, TRIES_TO_RECOVER);
                return -1;
            }
        } else {
            break;
        }
    } while (TRUE);
    if (writtenFrames > 0) {
        info->isFlushed = 0;
    }
    ret =  (int) (writtenFrames * info->frameBytes);
    TRACE2("%s: wrote %d bytes.\n", __FUNCTION__, ret);
    return ret;
}


int doDrain(PcmInfo* info) {
    snd_pcm_state_t state;

    int ret = snd_pcm_drain(info->handle);
    if (ret != 0) {
        ERROR2("%s: snd_pcm_drain: %s\n", __FUNCTION__, snd_strerror(ret));
        return FALSE;
    }
    return TRUE;
}

void doFlush(PcmInfo* info, int isSource) {
    TRACE1("%s: start\n", __FUNCTION__);
    if (info->isFlushed) {
        return;
    }
    int ret = snd_pcm_drop(info->handle);
    if (ret != 0) {
        ERROR2("%s: snd_pcm_drop: %s\n", __FUNCTION__, snd_strerror(ret));
        return;
    }
    info->isFlushed = 1;
    if (info->isRunning) {
        ret = doStart(info, isSource);
    }
    return;
}

int doGetAvailBytes(PcmInfo* info, int isSource) {
    int ret;
    snd_pcm_state_t state = snd_pcm_state(info->handle);
    if (info->isFlushed || state == SND_PCM_STATE_XRUN) {
        ret = info->bufferBytes;
    } else {
        snd_pcm_sframes_t availFrames = snd_pcm_avail_update(info->handle);
        if (availFrames < 0) {
            ret = 0;
        } else {
            ret = (int) (availFrames * info->frameBytes);
        }
    }
    TRACE2("%s: %d bytes\n", __FUNCTION__, ret);
    return ret;
}

INT64 doGetBytePos(PcmInfo* info, int isSource, INT64 javaBytePos) {
    int ret;
    INT64 result = javaBytePos;
    snd_pcm_state_t state = snd_pcm_state(info->handle);

    if (!info->isFlushed && state != SND_PCM_STATE_XRUN) {
        snd_pcm_uframes_t availFrames = snd_pcm_avail(info->handle);
        if (availFrames < 0) {
            ERROR2("%s: snd_pcm_avail: %s\n", __FUNCTION__, snd_strerror(ret));
            result = javaBytePos;
        } else {
            int availBytes = availFrames * info->frameBytes;
            if (isSource){
                result =(INT64) (javaBytePos - info->bufferBytes + availBytes);
            } else {
                result = (INT64) (javaBytePos + availBytes);
            }
        }
    }
    return result;
}
