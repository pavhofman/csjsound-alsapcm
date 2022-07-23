#ifndef TYPES_INCLUDED
#define TYPES_INCLUDED

typedef unsigned char           UINT8;
typedef char                    INT8;
#ifdef _LP64
typedef int                     INT32;
typedef long                    INT64;
#else
typedef long                    INT32;
typedef long long               INT64;
#endif

typedef unsigned long           UINT_PTR;
typedef long                    INT_PTR;

#undef TRUE
#undef FALSE

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#endif  // TYPES_INCLUDED