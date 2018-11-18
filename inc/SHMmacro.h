/**************************************************************************//**
* @file
*     共通メモリ管理機能 マクロ定義
* <pre>
*
*    １    機能
*           コンパイルオプション"SHM_DEBUG"指定時は、
*           デバックのためにCLoggerの変わりにSTDOUT出力を行うためのマクロ定義
*           (DEBUG無しの場合は無効になる)
*
*    ２    履歴
*            rev001 : 新規作成
* </pre>
**//**************************************************************************/

#ifndef PZ1SHMSHMMACRO_H_
#define PZ1SHMSHMMACRO_H_

#include <string>

namespace SharedMemory
{
extern "C" {
    const std::string SYSTEM_NAME = "SharedMemory";
}
}

// デバック用メモリダンプマクロ c option SHM_DEBUG_DMPを指定してください
#define TOHEX(c) (((c) & 15) > 9 ? ('A' + ((c) & 15) - 10) : ('0'+((c) & 15)))
#define SHM_DEBUG_DMP(_exe, _table, _index, _addr, _size) {\
    char buf[80] = {0};\
    ::printf(#_exe" [%s] %ld\n", (_table), (_index));\
    for (size_t i = 0; i < (_size); i++) {\
        const unsigned long a = (unsigned long)(_addr);\
        const unsigned char c = *(reinterpret_cast<char*>(a + i));\
        if ((i & 15) == 0) snprintf(buf, sizeof(buf), "%016lx", a + i);\
        buf[((i & 15) << 1) + 19] = TOHEX(c >> 4);\
        buf[((i & 15) << 1) + 20] = TOHEX(c);\
        buf[(i & 15) + 54] = (c < 0x20 || c > 0x7e) ? '.' : c;\
        buf[16] = buf[18] = buf[51] =buf[53] = ' ';\
        buf[17] = buf[52] = ':';\
        if ((i & 15) == 15) ::printf("%s\n", buf);\
    }\
    for (size_t i = ((_size) & 15); i < 16; i++) {\
        buf[((i & 15) << 1) + 19] = buf[((i & 15) << 1) + 20] = ' ';\
        buf[(i & 15) + 54] = '.';\
    }\
    if (((_size) & 15) != 0) ::printf("%s\n", buf);\
}

// ログ定義
#include <PBase>
#define SHM_DEBUG_LOG(...) DEBUG_LOG(__VA_ARGS__)
#define SHM_ERROR_LOG(...) ERROR_LOG(__VA_ARGS__)
#define SHM_WARN_LOG(...)  WARN_LOG(__VA_ARGS__)
#define SHM_INFO_LOG(...)  INFO_LOG(__VA_ARGS__)
#define SHM_TRACE_LOG(...) TRACE_LOG(__VA_ARGS__)
#define SHM_LOCK_LOG(...)  TRACE_LOG(__VA_ARGS__)
#define SHM_START_LOG      TRACE_LOG("START")
#define SHM_END_LOG        TRACE_LOG("END")


#endif /* PZ1SHMSHMMACRO_H_ */
