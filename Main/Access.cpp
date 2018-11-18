/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能 アクセスクラス
* <pre>
*
*    １  機能
*          共通メモリ管理機能 アクセスクラス
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#include <Init/Initializer.h>
#include <Main/Access.h>
#include <Main/Connection.h>
#include <Main/Cursor.h>
#include <Manager/Entity.h>
#include <Manager/Transaction.h>
#include <map>
#include <string>

#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::std::string;
using ::Entity::rowid_t;

Connection Access::connection;
/**************************************************************************//**
*
*     関数名：共有メモリ管理機能初期化・システムモニタ用 (init)
* <pre>
*
*    １    機能
*            共有メモリ管理情報を割り当てて、エンティティ名称マスタへ登録
*            する
*
*    ２    引数
*            fileName  : 共有メモリ設定ファイル名     [入力]
*            dataPath : 共有メモリデータパス名       [入力]
*
*    ３    戻り値
*            EXECUTE_ERR : メモリ確保失敗
*            EXECUTE_OK  : 成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Access::init(const string& fileName, const string& dataPath) {

    Initializer::createMemory(fileName, dataPath);

}

/**************************************************************************//**
*
*     関数名：共有メモリ管理機能初期化・システムモニタ以外用 (init)
* <pre>
*
*    １    機能
*            エンティティ名称マスタから管理情報をとりだして仮想メモリに
*            割り当てる
*
*    ２    引数
*            dataPath : 共有メモリデータパス名       [入力]
*
*    ３    戻り値
*            EXECUTE_ERR : メモリ確保失敗
*            EXECUTE_OK  : 成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Access::init(const string& dataPath) {

    Initializer::attachMemory(dataPath);

}
/**************************************************************************//**
*
*     関数名：共有メモリ管理機能終了 (destroy)
* <pre>
*
*    １    機能
*            共有メモリ管理情報を全て開放し、終了可能状態とする
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_ERR : 開放失敗
*            EXECUTE_OK  : 開放成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Access::destroy() {
    connection.close();
    Initializer::detachMemory();
}

/**************************************************************************//**
*
*     関数名：コネクションオブジェクト取得 (getConnection)
* <pre>
*
*    １    機能
*            コネクションオブジェクトを新たに作成する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            コネクションオブジェクト(NULLは失敗)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Connection& Access::getConnection() {
    return connection;
}

/**************************************************************************//**
*
*     関数名：ガベージコレクション (executeGarbageCollection)
* <pre>
*
*    １    機能
*            共有メモリ管理領域のガベージコレクションを実施する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*            マイナス値  : 失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Access::executeGarbageCollection() {

    Transaction& trn = Transaction::getTrans();

    trn.getLock(Header::WRITE_LOCK);

    // プロセスの生存チェック
    for(trid_t trid = trn.trid_collecting; trid < trn.trid_next; trid++) {
        Transaction::Recode& tr = trn.getTransaction(trid);
        // IN_PROGRESS以外は処理しない。
        if(tr.status != Transaction::IN_PROGRESS)
            continue;
        // 対象プロセスの開始時刻を取得
        time_t time = Initializer::getProcTime(tr.pid);
        // プロセスが存在しない場合、ステータスを変更する。
        if(time == -1 || time != tr.pid_time) {
            tr.status = Transaction::ABORTED;
        }
    }
    trn.releaseLock();

    trn.getLock(Header::READ_LOCK);
    // IN_PROGRESSのTRIDの下限を確認する
    trid_t tridInProg = trn.trid_collecting;
    for(trid_t next = trn.trid_next; tridInProg < next; tridInProg++) {
        // IN_PROGRESSの場合でもpidに該当するプロセスがいない場合は
        // ABORTEDとみなす
        Transaction::Recode& tr = trn.getTransaction(tridInProg);
        if(tr.status == Transaction::IN_PROGRESS) break;
    }
    // 回収可能なTRIDの範囲を確認する
    trid_t newTridColl = trn.trid_collecting;
    for(trid_t next = trn.trid_next; newTridColl < next; newTridColl++) {
        Transaction::Recode& tr = trn.getTransaction(newTridColl);
        // IN_PROGRESSの場合でもpidに該当するプロセスがいない場合は
        // ABORTEDとみなす
        if(tr.status == Transaction::IN_PROGRESS
                || (tr.status == Transaction::COMMITTED && tridInProg < tr.trid_end))
                break; // IN_PROGRESSのTrから参照される可能性がある
    }
    trn.releaseLock();

    // 回収可能なTRIDを開放する
    for(auto it = Initializer::table_map.begin(); it != Initializer::table_map.end(); it++) {
        Entity* tbl = it->second;
        if(nullptr == tbl) {
            WARN_LOG("テーブル情報がnullです:" << it->first);
            continue;
        }

        tbl->getLock(Header::WRITE_LOCK);

        size_t collected = 0; // DEBUG
        size_t remained = 0;  // DEBUG
        for(rowid_t rowid = 0, end = tbl->used_end; rowid < end; rowid++) {
            Entity::Entry& ent = tbl->getEntry(rowid);
            if(trn.trid_collecting <= ent.xmin && ent.xmin < newTridColl) {
                Transaction::Recode& tr = trn.getTransaction(ent.xmin);

                trn.getLock(Header::READ_LOCK);
                if(tr.status != Transaction::COMMITTED) {
                    trn.releaseLock();
                    tbl->freeTuple(rowid);
                    collected++; // DEBUG
                    continue;
                }
                trn.releaseLock();
            }
            if(trn.trid_collecting <= ent.xmax && ent.xmax < newTridColl) {
                Transaction::Recode& tr = trn.getTransaction(ent.xmax);

                trn.getLock(Header::READ_LOCK);
                if(tr.status == Transaction::COMMITTED) {
                    trn.releaseLock();
                    tbl->freeTuple(rowid);
                    collected++; // DEBUG
                    continue;
                } else {
                    trn.releaseLock();
                    ent.xmax = TRID_MAX;
                }
            }
            trn.getLock(Header::READ_LOCK);
            if (trn.trid_collecting <= ent.lock && ent.lock < newTridColl)
                ent.lock = TRID_MAX;
            trn.releaseLock();
            remained++;
        }
        // DEBUG
        INFO_LOG("[GC] %s : total=%lu used_end=%lu free_begin=%lu "
                "collected=%lu remained=%lu",
                tbl->getName(), tbl->getMaxLine(), tbl->used_end,
                tbl->free_begin, collected, remained);
        tbl->releaseLock();
    }
    trn.getLock(Header::WRITE_LOCK);
    trn.trid_collecting = newTridColl;
    trn.releaseLock();

    return;
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}   // namespace SharedMemory
