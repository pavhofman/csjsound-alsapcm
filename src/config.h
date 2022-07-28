#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED

#define USE_ERROR
//#define USE_TRACE

// maximum string length (deviceID, name, description)
#define STR_LEN                 200

// period time for buffers larger than SMALL_BUFFER_SIZE_LIMIT frames
#define DEFAULT_PERIOD_TIME     20000

#define SMALL_BUFFER_SIZE_LIMIT 1024

#define TRIES_TO_RECOVER        3

// config names ignored when enumerating pcm devices
static const char *IGNORED_CONFIGS[] = {
			"hw", "plughw", "plug", "dsnoop", "tee",
            "file", "null", "shm", "cards", "rate_convert",
            NULL
};

#endif // CONFIG_INCLUDED
