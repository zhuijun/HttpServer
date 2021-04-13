#ifndef HEADER_H__
#define HEADER_H__

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>

#elif defined __APPLE__

#include <sys/time.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
/* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#else

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
/* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif // _WIN32

#include <time.h>
#include <stdint.h>

inline int64_t nowtick()
{
#ifdef _WIN32
    static int64_t ts_begin_ = time(NULL);
    static  int64_t tick_begin_ = ::GetTickCount64();
    return ::GetTickCount64() - tick_begin_ + ts_begin_ * 1000;
#elif defined __APPLE__
    timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
#else
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
}


#ifndef _WIN32
typedef unsigned int       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef long long LONGLONG;
typedef unsigned int       ULONG;

#define FALSE 0
#define TRUE 1
#define KERNEL_ASSERT
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SD_BOTH SHUT_RDWR
#define closesocket close

#if defined (_M_X64) || defined (_M_ARM)
#define _UNALIGNED __unaligned
#else  /* defined (_M_X64) || defined (_M_ARM) */
#define _UNALIGNED
#endif  /* defined (_M_X64) || defined (_M_ARM) */

//#define _countof(a) sizeof(a)/sizeof(a[0])
/* _countof helper */
#if !defined (_countof)
#if !defined (__cplusplus)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else  /* !defined (__cplusplus) */
extern "C++"
{
    template <typename _CountofType, size_t _SizeOfArray>
    char(*__countof_helper(_UNALIGNED _CountofType(&_Array)[_SizeOfArray]))[_SizeOfArray];
#define _countof(_Array) (sizeof(*__countof_helper(_Array)) + 0)
}
#endif  /* !defined (__cplusplus) */
#endif  /* !defined (_countof) */

#endif

#endif
