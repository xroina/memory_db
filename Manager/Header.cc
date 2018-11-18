/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能 基盤共有メモリ構造体定義
* <pre>
*
*    １  機能
*          共通メモリ管理機能 基盤共有メモリ構造体を定義する
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/

#include <Manager/Header.h>
#include <cstring>
#include <unistd.h>
#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::std::string;

const string Header::FILE_HEADER = "SHM::";
const string Header::FILE_EXP = ".table";

/**************************************************************************//**
*
*     関数名：共通メモリ管理機能 初期化 (init)
* <pre>
*
*    １    機能
*          初期化を実施する
*
*    ２    引数
*            _name      :   領域名                  [入力]
*            _file      :   ロックファイル名        [入力]
*
*    ３    戻り値
*            なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Header::init(const string& memName, const msec_t to,
        const size_t maxLine, const size_t memSize, const size_t uSize) {

    name = memName;
    lock_count = 0;
    lock_status = UNLOCK;

    time_out = to;
    max_line = maxLine;
    size = memSize;
    unit_size = uSize;
}

/**************************************************************************//**
*
*     関数名：ファイルロックオブジェクト取得 (getLock)
* <pre>
*
*    １    機能
*            エンティティ名からロックファイルを取得してロックを取得し、
*            対応するロックオブジェクトを利用可能にする
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            OK  : 正常終了
*            NG  : 異常終了
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Header::getLock(Lock status) {

    if(status != READ_LOCK && status != WRITE_LOCK)
        LOCK_FAILED("ロックの取得に失敗しました：" << name);

    // WRITE_LOCKに変える時以外はカウントするだけにする
    if(lock_count != 0) {
        lock_count++;
        // WRITE->WRITE, WRITE->READは変えない
        if(lock_status == WRITE_LOCK) return;
        // READ->READも変えない
        if(lock_status == READ_LOCK && status == READ_LOCK) return;
    } else {
        this->lock_status = UNLOCK; // ロック種別：解除
        struct stat buf;            // ファイルステータス取得
        if(::fstat(this->fd, &buf) != 0) buf.st_size = -1;
        long fsize = buf.st_size;
        if(fsize < 0) {
            releaseLock();
            LOCK_FAILED("情報取得に失敗しました：" << name);
        }
        // ロック構造体情報設定
        flck.l_whence = SEEK_SET; // ファイル最初から
        flck.l_start = 0;         // ファイルの0バイト目から
        flck.l_len = fsize;       // ファイルサイズ
        flck.l_pid = getpid();
        lock_count = 1;
    }

    lock_status = status;

    // ロックを取得するまで待つ設定でロックの取得
    if(::fcntl(fd, F_SETLKW, &flck) < 0) {
        releaseLock();
        LOCK_FAILED("ロックの取得に失敗しました：" << name);
    }
    SHM_LOCK_LOG("fd:%d type:%d count:%d", fd, flck.l_type, lock_count);

    return;
}

/**************************************************************************//**
*
*     関数名：ファイルロックオブジェクト開放 (releaseLock)
* <pre>
*
*    １    機能
*            ファイルロックをアンロックにする
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK  : 正常終了
*            EXECUTE_ERR : 異常終了
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Header::releaseLock() {
    if(lock_count == 0) return;
    if(lock_status == UNLOCK) return;

    lock_count--;
    if(lock_count > 0) return;
    // ロック構造体情報設定
    lock_status = UNLOCK;   // ロック種別：解除
    // その他のパラメータはopen時に作られている
    // ロックを解除する。
    if(::fcntl(fd, F_SETLK, &flck) < 0)
        LOCK_FAILED("ロックの解除に失敗しました：" << name);

    lock_count = 0;
    SHM_LOCK_LOG("fd:%d type:%d count:%d", fd, flck.l_type, lock_count);
    return;
}

/**************************************************************************//**
*
*     関数名：共通メモリ管理機能 アタッチ情報のログ出力
* <pre>
*
*    １    機能
*          アタッチ情報のログ出力を実施する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Header::attatchLog() const {
    INFO_LOG("アタッチ情報:%p Name:%s / MaxLine:%lu / TimeOut:%lu / "
            "MemorySize:%lu / UnitSize:%lu", this, name.c_str(), max_line,
            time_out, size, unit_size);
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // end namespace SharedMemory
