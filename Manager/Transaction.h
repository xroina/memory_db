/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能 全体トランザクション管理構造体定義
* <pre>
*
*    １  機能
*          共通メモリ管理機能 全体トランザクション管理構造体を定義する
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/

#ifndef SHAREDMEMORY_TRANSACTION_H_
#define SHAREDMEMORY_TRANSACTION_H_

#include <Manager/Header.h>
#include <unistd.h>
#include "inc/SHMConst.h"

namespace SharedMemory
{
typedef uint64_t trid_t;    ///< Tr識別子型(64ビット符号なし整数)
static const trid_t  TRID_MAX = ~0uL;     ///< 最大値
static const trid_t  TRID_MIN =  0uL;     ///< 最小値

typedef uint64_t trcc_t;    ///< Trコミットカウント(TRCC)型(64ビット符号なし整数)
static const trcc_t  TRCC_MAX = ~0uL;     ///< TRCC 最大値
static const trcc_t  TRCC_MIN =  0uL;     ///< TRCC 最小値

/**************************************************************************//**
*
*     クラス名：共通メモリ管理機能 全体管理領域（全体トランザクション管理領域）
*     (CTransactionManager)
* <pre>
*
*    １    機能
*          全体管理領域（トランザクション管理領域の構造を定義する
*
*    ２    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
class Transaction : public Header {
public:

    trid_t trid_next;           ///< 次のTRID
    trid_t trid_collecting;     ///< 最古の未回収TRID
    trcc_t trcc_next;           ///< 次のTRCC
    ::Entity::rowid_t index_root_master;  ///< インデックスルートマスタ

    /**********************************************************************//**
    *     構造体名：共通メモリ管理機能 全体トランザクション管理情報定義
    **//**********************************************************************/
    enum Status { IN_PROGRESS, COMMITTED, ABORTED };
    class Recode {
    public:
        trid_t trid_end;        ///< Tr終了時のTRID現在値
        trcc_t trcc_begin;      ///< 処理開始時のTRCC現在値
        trcc_t trcc_end;        ///< コミット時のTRCC現在値
        Status status;          ///< トランザクション状態
        pid_t  pid;             ///< Tr実行プロセスのPID
        time_t pid_time;        ///< Tr実行プロセスの開始時間
        ::Entity::rowid_t  index_root;    ///< インデックス管理テーブルの基底
    };
    Recode tag_transaction[0];    ///< トランザクション管理配列

    static const ::std::string TRANSACTION_NAME;

public:
    /// サイズ取得
    static size_t getSize(size_t);
    /// 全体トランザクション管理情報取得
    static Transaction& getTrans();
    /// 初期化
    void init(const ::std::string&, const msec_t, const size_t);
    /// トランザクション管理情報アドレス取得
    Recode& getTransaction(trid_t);
    /// トランザクション開始
    trid_t startTr();
    /// トランザクションコミット
    void commitTr(trid_t);
    /// トランザクションアボート
    void abortTr(trid_t);
    /// トランザクション可視判定
    static bool is_tr_valid_to_read(trid_t, trid_t);
    /// トランザクション対象ID
    typedef enum {IS_XMAX, IS_LOCK} target_trid_t;
    /// xmaxとlockのトランザクション判定
    static bool is_tr_valid_to_write(trid_t, trid_t, target_trid_t);
};
/*--------1---------2---------3---------4---------5---------6---------7------*/
}   // end namespace ShahedMemory

#endif /* SHAREDMEMORY_TRANSACTION_H_ */
