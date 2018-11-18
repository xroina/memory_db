/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能 基盤共有メモリ構造ヘッダ
* <pre>
*
*    １  機能
*          共通メモリ管理機能 基盤共有メモリ構造ヘッダを定義する
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#ifndef SHAREDMEMORY_HEADER_H_
#define SHAREDMEMORY_HEADER_H_

//#define PTHREAD_MUTEX
#include <Entity/AppTable.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <string>

#include "inc/SHMConst.h"

namespace SharedMemory
{
typedef uint64_t msec_t;                        ///< ミリ秒(64ビット符号なし整数)
static const msec_t INVALID_MSEC  = ~0uL;       ///< ミリ秒無効値

/**************************************************************************//**
*     クラス名：共通メモリ管理機能 基盤共有メモリ構造ヘッダ
*     (CSharedMemoryHeader)
* <pre>
*          共通メモリ管理機能 基盤共有メモリ構造ヘッダ定義
* </pre>
**//**************************************************************************/
class Header {

private:
    size_t size;                                    ///< メモリサイズ
    int    fd;                                      ///< ファイルディスクプリタ
    ::Entity::entityname_t   name;      ///< エンティティ名
    flock  flck;                                    ///< flock テーブル

public:
    /// ロック種別
    enum Lock {
        READ_LOCK  = F_RDLCK,
        WRITE_LOCK = F_WRLCK,
        UNLOCK     = F_UNLCK
    };

private:
    Lock lock_status;                               ///< ロック種別
    int lock_count;                                 ///< ロックカウンタ

    msec_t time_out;                                ///< タイムアウト時間(ms)
    size_t max_line;                                ///< 要素数
    size_t unit_size;                               ///< ユニットサイズ

    /// コントラクタ(無効)
    Header();

public:
    static const ::std::string FILE_HEADER;
    static const ::std::string FILE_EXP;

    /// 初期化
    void init(const ::std::string&, const msec_t,
            const size_t, const size_t, const size_t);

    /// ロック取得
    void getLock(Lock);
    /// ロック開放
    void releaseLock();

    /// 割り当てログ採取
    void attatchLog() const;

    /**********************************************************************//**
    *     関数名：ファイルディスクプリタ取得(getFileDiscpriter)
    * <pre>
    *           管理テーブルに保持しているファイルディスクプリタを取得する
    *    引数   : なし
    *    戻り値 : ファイルディスクプリタ
    * </pre>
    **//**********************************************************************/
    inline int getFileDiscpriter() const {
        return fd;
    }

    /**********************************************************************//**
    *     関数名：ファイルディスクプリタ設定(setFileDiscpriter)
    * <pre>
    *           管理テーブルに保持しているファイルディスクプリタを設定する
    *    引数   : ファイルディスクプリタ
    *    戻り値 : なし
    * </pre>
    **//**********************************************************************/
    inline void setFileDiscpriter(const int fd) {
        this->fd = fd;
    }

    /**********************************************************************//**
    *     関数名：タイムアウト時間取得(getTimeOut)
    * <pre>
    *           管理テーブルに保持しているタイムアウト時間(ミリ秒)を取得する
    *    引数   : なし
    *    戻り値 : タイムアウト時間(ミリ秒)
    * </pre>
    **//**********************************************************************/
    inline msec_t getTimeOut() const {
        return time_out;
    }

    /**********************************************************************//**
    *
    *     関数名：管理領域データ最大数取得(getMaxLine)
    * <pre>
    *     管理テーブルに保持しているデータ最大数を取得する
    *    引数 : なし
    *    戻り値 : データ数
    * </pre>
    **//**********************************************************************/
    inline size_t getMaxLine() const {
        return max_line;
    }

    /**********************************************************************//**
    *     関数名：管理領域ユニットサイズ取得(getUnitSize)
    * <pre>
    *           管理テーブルに保持しているユニットサイズを取得する
    *    引数   : なし
    *    戻り値 : ユニットサイズ(bytes)
    * </pre>
    **//**********************************************************************/
    inline size_t getUnitSize() const {
        return unit_size;
    }

    /**********************************************************************//**
    *     関数名：管理領域ユニットサイズ取得(getMemorySize)
    * <pre>
    *           管理テーブルに保持しているユニットサイズを取得する
    *    引数   : なし
    *    戻り値 : ユニットサイズ(bytes)
    * </pre>
    **//**********************************************************************/
    inline size_t getMemorySize() const {
        return size;
    }

    /**********************************************************************//**
    *     関数名：管理領域メモリ名称取得(getMemoryName)
    * <pre>
    *           管理領域に保持しているメモリ名称を取得する
    *    引数   : なし
    *    戻り値 : メモリ名称
    * </pre>
    **//**********************************************************************/
    inline const ::std::string getName() const {
        return name;
    }
};

}  // end namespace SharedMemory

#endif /* SHAREDMEMORY_HEADER_H_ */
