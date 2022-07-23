#ifndef DEBUG_INCLUDED
#define DEBUG_INCLUDED

#ifdef USE_ERROR
#define ERROR0(string)                        { fprintf(stdout, (string)); fflush(stdout); }
#define ERROR1(string, p1)                    { fprintf(stdout, (string), (p1)); fflush(stdout); }
#define ERROR2(string, p1, p2)                { fprintf(stdout, (string), (p1), (p2)); fflush(stdout); }
#define ERROR3(string, p1, p2, p3)            { fprintf(stdout, (string), (p1), (p2), (p3)); fflush(stdout); }
#define ERROR4(string, p1, p2, p3, p4)        { fprintf(stdout, (string), (p1), (p2), (p3), (p4)); fflush(stdout); }
#define ERROR4(string, p1, p2, p3, p4)        { fprintf(stdout, (string), (p1), (p2), (p3), (p4)); fflush(stdout); }
#define ERROR5(string, p1, p2, p3, p4, p5)    { fprintf(stdout, (string), (p1), (p2), (p3), (p4), (p5)); fflush(stdout); }
#else
#define ERROR0(string)
#define ERROR1(string, p1)
#define ERROR2(string, p1, p2)
#define ERROR3(string, p1, p2, p3)
#define ERROR4(string, p1, p2, p3, p4)
#define ERROR5(string, p1, p2, p3, p4, p5)
#endif

#ifdef USE_TRACE
#define TRACE0(string)                        { fprintf(stdout, (string)); fflush(stdout); }
#define TRACE1(string, p1)                    { fprintf(stdout, (string), (p1)); fflush(stdout); }
#define TRACE2(string, p1, p2)                { fprintf(stdout, (string), (p1), (p2)); fflush(stdout); }
#define TRACE3(string, p1, p2, p3)            { fprintf(stdout, (string), (p1), (p2), (p3)); fflush(stdout); }
#define TRACE4(string, p1, p2, p3, p4)        { fprintf(stdout, (string), (p1), (p2), (p3), (p4)); fflush(stdout); }
#else
#define TRACE0(string)
#define TRACE1(string, p1)
#define TRACE2(string, p1, p2)
#define TRACE3(string, p1, p2, p3)
#define TRACE4(string, p1, p2, p3, p4)
#endif

#endif  // DEBUG_INCLUDED