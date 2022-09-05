# csjsound-alsapcm
Alsa PCM dynamic library for https://github.com/pavhofman/csjsound-provider . Only PCM devices are listed (as opposite to the standard OpenJDK alsa provider which lists only hw cards).

## Building
For amd64:
```
cd src
./compile.sh
```

Cross compiling on ubuntu for aarch64 (requires the aarch64 build chain and libasound2:arm64 library):

```
./compile.sh aarch64
```
 ## Logging
 The log level is defined at compile time by defining USE_ERROR and/or USE_TRACE in https://github.com/pavhofman/csjsound-alsapcm/blob/8b738ad20c9a0569d936d31d32c1311a81632c92/src/config.h#L4.
 
 All logs are sent to stdout (via standard fprintf(stdout)) https://github.com/pavhofman/csjsound-alsapcm/blob/8b738ad20c9a0569d936d31d32c1311a81632c92/src/debug.h#L4
 
## Detected Formats
The javasound API requires a list of pre-determined formats supported by the devices. The ALSA-PCM library reads hw_params and sends combinations of:

* min/max rates
https://github.com/pavhofman/csjsound-alsapcm/blob/8b738ad20c9a0569d936d31d32c1311a81632c92/src/impl.c#L269
* min/2/max channels
https://github.com/pavhofman/csjsound-alsapcm/blob/8b738ad20c9a0569d936d31d32c1311a81632c92/src/impl.c#L179
Because only interval rate/channel values are reported, a format with channels = AudioSystem.NOT_SPECIFIED (-1) is added for each combination.

## Ignored Config Names
The alsa configs enumeration skips standard config names, same as in PortAudio https://github.com/pavhofman/csjsound-alsapcm/blob/8b738ad20c9a0569d936d31d32c1311a81632c92/src/config.h#L20

