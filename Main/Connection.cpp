/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能コネクションクラス
* <pre>
*
*    １  機能
*          共通メモリ管理機能コネクションクラスを定義する
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#include <Init/Exception.h>
#include <Init/Initializer.h>
#include <Main/Connection.h>
#include <Main/Cursor.h>
#include <Manager/IndexManager.h>
#include <Manager/Transaction.h>
#include <sys/time.h>

#include <string>

#include "Entity/ImplMatcher.h"
#include "Entity/IndexerCache.h"

#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::std::string;
using ::Entity::AppTable;
using ::Entity::ImplMatcher;
using ::Entity::ImplSorter;
using ::Entity::AbstIndexMatcher;

/**************************************************************************//**
*
*     関数名：コンストラクタ
* <pre>
*
*    １    機能
*            インスタンスのトランザクションIDを初期化する
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
Connection::Connection() {
     this->trid = TRID_MAX;
     this->cursor_vct.clear();
     // リードコミットで初期化
     this->level = READ_COMMITTED;
}

/**************************************************************************//**
*
*     関数名：デストラクタ
* <pre>
*
*    １    機能
*            特になし
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
Connection::~Connection() {
//    this->close();
}

/**************************************************************************//**
*
*     関数名：カーソルオープン (openCursor)
* <pre>
*
*    １    機能
*            指定した条件に沿ったカーソルオブジェクトを作成する
*
*    ２    引数
*            data           : フェッチ対象のデータ
*            bLockFlag      : 更新ロック取得フラグ
*            index_name    : インデックスID
*            IndexMatcher   : インデックスマッチャ
*            DefaultMatcher : デフォルトマッチャ
*            Sorter         : ソータ
*
*    ３    戻り値
*            カーソルオブジェクトを返却する
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Cursor& Connection::openCursor(AppTable& data, const bool flag,
        AbstIndexMatcher* idxMtcr, const ImplMatcher* dftMtcr,
        const ImplSorter* sorter) {

    Cursor* cur = new Cursor(data);     // カーソルオブジェクト作成
    cursor_vct.push_back(cur);

    // トランザクションを取得
    getTransaction();

    // 更新ロックフラグがONなら更新ロックをとる
    if(flag) {
        // TODO(ロック開放待ちはここで行う必要がある)
        for(msec_t start = msecGet(); timeCheck(start); sleep()) {
            cur->setErrorCode(EXECUTE_TIMEOUT);
            // トランザクション調整
            adjustTransaction();
            // index_rootの更新ロックを取得
            if(IndexManager::lock_index_root(data.getTableName(), trid)) {
                try {
                    // インデックス検索処理
                    IndexManager::search_tuples(cur->getRowIDs(), flag,
                            trid, data.getTableName(), idxMtcr, dftMtcr, sorter);
                    // リターンコードがタイムアウトなら引き続きスリープする
                } catch(::Exception::timeout& e) {
                    continue;
                }
                cur->setErrorCode(EXECUTE_OK);
                break;
            }
        }
    } else {
        // トランザクション調整
        adjustTransaction();
        // インデックス検索処理
        IndexManager::search_tuples(cur->getRowIDs(), flag,
                trid, data.getTableName(), idxMtcr, dftMtcr, sorter);
    }

    return *cur;
}

/**************************************************************************//**
*
*     関数名：データ挿入 (executeInsert)
* <pre>
*
*    １    機能
*            指定したデータを対象のエンティティへ挿入する。
*
*    ２    引数
*            szEntityName   : エンティティ名
*            data           : 挿入対象のデータ
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
int Connection::executeInsert(AppTable& data) {

    // トランザクションを取得
    getTransaction();

    // TODO(ロック開放待ちはここで行う必要がある)
    for(msec_t start = msecGet(); timeCheck(start); sleep()) {
        // トランザクション調整
        adjustTransaction();
        // index_rootの更新ロックを取得
        if(IndexManager::lock_index_root(data.getTableName(), trid)) {
            try {
                // データ挿入処理
                IndexManager::insert_tuple(trid, data.getTableName(),
                        data.getData(), data.getTableSize());//TODO
            } catch(::Exception::timeout& e) {
                continue;
            }
            return EXECUTE_ONE;
            break;
        }
    }

    return EXECUTE_TIMEOUT;
}

/**************************************************************************//**
*
*     関数名：データ更新 (executeUpdate)
* <pre>
*
*    １    機能
*            指定した条件に沿ったデータを削除し、指定したデータを挿入する
*
*    ２    引数
*            szEntityName   : エンティティ名
*            szIndexName    : インデックスID
*            IndexMatcher   : インデックスマッチャ
*            DefaultMatcher : デフォルトマッチャ
*            data           : 更新対象のデータ
*
*    ３    戻り値
*            正の数     : 成功。値は条件にマッチしたデータ数(削除データ数)
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
int Connection::executeUpdate(AppTable& data, AbstIndexMatcher* idxMtcr,
        const ImplMatcher* dftMtcr) {

    // トランザクションを取得
    getTransaction();

    // TODO(ロック開放待ちはここで行う必要がある)
    for(msec_t start = msecGet(); timeCheck(start); sleep()) {
        // トランザクション調整
        adjustTransaction();
        // index_rootの更新ロックを取得
        if(IndexManager::lock_index_root(data.getTableName(), trid)) {
            try {
                // 対象を削除
                IndexManager::delete_tuples(trid, data.getTableName(), idxMtcr, dftMtcr);
                // 削除が成功したら、指定データを追加登録
                IndexManager::insert_tuple(trid, data.getTableName(),
                        data.getData(), data.getTableSize());//TODO
                // 戻り値がエラーだったらその戻り値で上書きする
            } catch(::Exception::timeout& e) {
                continue;
            }
            return EXECUTE_OK;
        }
    }

    return EXECUTE_TIMEOUT;
}
/**************************************************************************//**
*
*     関数名：データ削除 (executeDelete)
* <pre>
*
*    １    機能
*            指定した条件に沿ったデータを削除する
*
*    ２    引数
*            szEntityName   : エンティティ名
*            szIndexName    : インデックスID
*            IndexMatcher   : インデックスマッチャ
*            DefaultMatcher : デフォルトマッチャ
*
*    ３    戻り値
*            正の数     : 成功。値は条件にマッチしたデータ数(削除データ数)
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
int Connection::executeDelete(const string& entName, AbstIndexMatcher* idxMtcr,
        const ImplMatcher* dftMtcr) {

    getTransaction();

    // TODO(ロック開放待ちはここで行う必要がある)
    for(msec_t start = msecGet(); timeCheck(start); sleep()) {
        // トランザクション調整
        adjustTransaction();
        // index_rootの更新ロックを取得
        if(IndexManager::lock_index_root(entName, trid)) {
            try {
                // 対象を削除
                IndexManager::delete_tuples(trid, entName, idxMtcr, dftMtcr);
            } catch(::Exception::timeout& e) {
                continue;
            }
            return EXECUTE_OK;
        }
    }

    return EXECUTE_TIMEOUT;
}

/**************************************************************************//**
*
*     関数名：トランザクションコミット (commitTransaction)
* <pre>
*
*    １    機能
*            トランザクションを確定する。
*            トランザクションが無くてもなにもしない
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Connection::commitTransaction() {
    if(trid != TRID_MAX) {
        Transaction& trn = Transaction::getTrans();
        trn.getLock(Header::WRITE_LOCK);
        trn.commitTr(trid);
        trn.releaseLock();
    }
    trid = TRID_MAX;
}
/**************************************************************************//**
*
*     関数名：トランザクションロールバック (rollbackTransaction)
* <pre>
*
*    １    機能
*            トランザクションを無効化する。
*            トランザクションが無くてもなにもしない
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Connection::rollbackTransaction() {
    if(trid != TRID_MAX) {
        Transaction& trn = Transaction::getTrans();
        trn.getLock(Header::WRITE_LOCK);
        trn.abortTr(trid);
        trn.releaseLock();
    }
    trid = TRID_MAX;
}
/**************************************************************************//**
*
*     関数名：コネクション切断(close)
* <pre>
*
*    １    機能
*            自インスタンスを削除してコネクションを切断したことにする
*            実際に接続があるわけではない
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Connection::close() {
    // トランザクションが確定していなければ、ロールバックで確定させる
    if(trid != TRID_MAX) rollbackTransaction();

    // 開いているカーソルがあったら全て閉じる
    for(auto i = cursor_vct.begin(); i != cursor_vct.end(); i++) {
        (*i)->close();
        delete *i;
        *i = nullptr;
    }
    this->cursor_vct.clear();

    this->trid = TRID_MAX;

}

/**************************************************************************//**
*
*     関数名：トランザクション分離レベル設定(setIsolationIevel)
* <pre>
*
*    １    機能
*            トランザクションの分離レベルを、READ_COMMITTEDかSERIALIZABLE
*            に設定する。ディフォルトはSERIALIZABLEである
*            トランザクションの開始以降に使用するとNGを返却する
*
*    ２    引数
*            level     : READ_COMMITTED
*                        SERIALIZABLE
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Connection::setIsolationLevel(IsolationLevel level) {

    // トランザクションを実行ではない場合だけ変更可能
    if(trid != TRID_MAX) TRANSACTION_MISMATCH("トランザクションが開始されています");

    this->level = level;

    return;
}

/**************************************************************************//**
*
*     関数名：トランザクション分離レベル取得(getIsolationIevel)
* <pre>
*
*    １    機能
*            トランザクションの分離レベルを、取得する。
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            READ_COMMITTED : リードコミッティッド
*            SERIALIZABLE   : シリアライズブル
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
const Connection::IsolationLevel Connection::getIsolationLevel() const {
    return level;
}

/**************************************************************************//**
*
*     関数名：トランザクションID取得(getTransaction)
* <pre>
*
*    １    機能
*            トランザクションIDを取得する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Connection::getTransaction() {
    // トランザクション取得済みなら無条件でOKを返す
    if(this->trid != TRID_MAX) return;

    Transaction& trn = Transaction::getTrans();

    for(msec_t start = msecGet(); timeCheck(start); sleep()) {
        // 全体管理領域で排他ロックを取得する
        trn.getLock(Header::WRITE_LOCK);
        // collectingとnextの差がmax_line以下ならトランザクション取得可能
        if(trn.trid_next - trn.trid_collecting < trn.getMaxLine()) {
            // 新しいトランザクションの取得
            trid = trn.startTr();
            // ロックを開放する
            trn.releaseLock();

            return;
        }
        // ロックを開放する
        trn.releaseLock();
        // 取得不能の場合はスリープしてループ
    }
    TIMEOUT("トランザクションタイムアウト");
}

/**************************************************************************//**
*
*     関数名：トランザクション調整処理(adjustTransaction)
* <pre>
*
*    １    機能
*           Read Committed時のトランザクション調整処理
*           インデックスルート、コミットカウント現在値をとりなおす
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値 : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Connection::adjustTransaction() {
    if(level != READ_COMMITTED) return;
    Transaction& trn = Transaction::getTrans();
    Transaction::Recode& tr = trn.getTransaction(trid);

    // 全体管理領域で排他ロックを取得する
    trn.getLock(Header::WRITE_LOCK);
    // コミットカウント現在値の保存
    tr.trcc_begin = trn.trcc_next;
    // インデックスルート書き換え
    if(IndexManager::is_index_root_valid(trid, trn.index_root_master)) {
        TRACE_LOG("(trid:" << trid << ") MASTERroot:" << trn.index_root_master
                << " -> TRNroot:" << tr.index_root);
        tr.index_root = trn.index_root_master;
    }
    // ロックを開放する
    trn.releaseLock();

    return;
}

/**************************************************************************//**
*
*     関数名：システム時間取得 (msecGet)
* <pre>
*
*    １    機能
*            システムの内部時計を参照し、ミリ秒単位で返却する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            システム時間 (ミリ秒)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
msec_t Connection::msecGet() {
    static struct timeval tv = {0, 0};  // 開始時刻構造体定義
    ::gettimeofday(&tv, nullptr);
    return (msec_t)tv.tv_sec * 1000uL + (msec_t)tv.tv_usec / 1000uL;
}

/**************************************************************************//**
*
*     関数名：タイムアウトチェック関数 (timeCheck)
* <pre>
*
*    １    機能
*            システムの内部時計を参照し、ミリ秒単位で返却する
*
*    ２    引数
*            start  : 開始時刻
*
*    ３    戻り値
*            false  : タイムアウト
*            true   : タイムアウトではない
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
bool Connection::timeCheck(msec_t start) {
    Transaction& trn = Transaction::getTrans();
    return (msecGet() - start) < trn.getTimeOut() || trn.getTimeOut() == 0;
}

void Connection::sleep() {
    ::usleep(Transaction::getTrans().getTimeOut() * 100);
}

/*--------1---------2---------3---------4---------5---------6---------7------*/

}   // namespace SharedMemory
