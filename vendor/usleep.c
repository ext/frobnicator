#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

void usleep (unsigned __int64 usec){
        LARGE_INTEGER lFrequency;
        LARGE_INTEGER lEndTime;
        LARGE_INTEGER lCurTime;

        QueryPerformanceFrequency (&lFrequency);
        if (lFrequency.QuadPart){
                QueryPerformanceCounter (&lEndTime);
                lEndTime.QuadPart += (LONGLONG) usec * lFrequency.QuadPart / 1000000;
                do {
                        QueryPerformanceCounter (&lCurTime);
                        Sleep(0);
                } while (lCurTime.QuadPart < lEndTime.QuadPart);
        }
}
