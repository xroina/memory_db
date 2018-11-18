/**************************************************************************//**
* @file
*     モジュール名：インデックス管理情報クラス
* <pre>
*
*    １  機能
*          インデックス管理情報クラス
*
*    ２  関数名一覧
*           サイズ取得                 (getSize)
*           初期化                     (init)
*           データ検索本体             (search_tuples)
*           データ挿入本体             (insert_tuple)
*           データ削除本体             (delete_tuples)
*           インデックス開始位置の取得 (load_index_root)
*           インデックス開始位置の保存 (store_index_root)
*
*    ３  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#include <string>
#include <algorithm>
#include <cstring>

#include <Init/Exception.h>
#include <Init/Initializer.h>
#include <Entity/IndexerCache.h>
#include <Entity/IndexName.h>
#include <Manager/IndexManager.h>
#include <Manager/Transaction.h>

#include "inc/SHMConst.h"
#include "inc/SHMmacro.h"

namespace SharedMemory {

using ::std::string;
using ::Entity::ImplMatcher;
using ::Entity::AbstIndexMatcher;
using ::Entity::ImplSorter;
using ::Entity::ImplIndexer;
using ::Entity::AbstEntity;
using ::Entity::IndexName;
using ::Entity::IndexNameIndexer;
using ::Entity::rowid_t;
using ::Entity::INVALID_ROWID;
using ::Entity::rowid_vec_t;
using ::Entity::IndexerCache;
using ::Entity::IndexNameMatcher;

/**************************************************************************//**
*
*     関数名：データ検索本体 (search_tuples)
* <pre>
*
*    １    機能
*            指定されたエンティティをマッチャで検索し、ソーターで並び替える
*
*    ２    引数
*            p_rowid_vec        : 検索結果                    [出力]
*            lock_flag          : ロックフラグ                [入力]
*            trid               : トランザクションID          [入力]
*            table_name         : エンティティ名              [入力]
*            index_matcher      : インデックスマッチャ        [入力]
*            default_matcher    : デフォルトマッチャ          [入力]
*            sorter             : ソータ                      [入力]
*
*    ３    戻り値
*            ０以上    : 成功
*            マイナス値 : 初期化失敗
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void IndexManager::search_tuples(rowid_vec_t& rows, const bool flag, const trid_t trid,
        const string& tblName, AbstIndexMatcher* idxMtcr,
        const ImplMatcher* dftMtcr, const ImplSorter* sorter) {

    if(trid == TRID_MAX) TRANSACTION_MISMATCH("トランザクションが開始されていません");

    // テーブル名よりエンティティを取得
    Entity& tbl = Entity::getAddr(tblName);

    // 検索
    if(idxMtcr != nullptr) {
        IndexName tpl;
        // インデックスルート取得
        load_index_root(trid, tbl, idxMtcr->getIndexName(), tpl);
        // インデックス名よりインデックスエンティティ取得
        Index& index = Index::getAddr(tpl.index_name);

        // 全体管理領域で共有ロックを取得する
        Transaction::getTrans().getLock(Header::READ_LOCK);

        // 検索実行
        index.searchNodes(rows, flag, trid, tpl.index_root, tbl, idxMtcr, dftMtcr);

        // 全体管理領域ロック解除
        Transaction::getTrans().releaseLock();
    } else {
        // インデックスなしの全検検索
        for(rowid_t rowid = 0; rowid < tbl.used_end; rowid++) {
            Entry& entry = tbl.getEntry(rowid);
            // 全体管理領域で共有ロックを取得する
            Transaction::getTrans().getLock(Header::READ_LOCK);

            // エントリの可視判定
            if(!Entity::check_tuple_readable(trid, entry)) {
                // 不可視の場合はロック外して次ループへ
                Transaction::getTrans().releaseLock();
                continue;
            }
            const ::Entity::AbstEntity& adr = tbl.getTuple(rowid);
            if(dftMtcr != nullptr && dftMtcr->match(adr) != 0) {
                // マッチャがあってマッチしない場合はロック外して次ループへ
                Transaction::getTrans().releaseLock();
                continue;
            }
            // 更新ロックありの場合
            if(flag) {
                // 更新可否チェック
                if(Entity::check_tuple_writable(trid, entry) == LOCKED) {
                    // 更新不能ならばリソース開放してループを抜ける
                    rows.clear();
                    Transaction::getTrans().releaseLock();
                    // 上位でタイムアウト処理してもらう
                    TIMEOUT(tbl.getName() << " Update TimeOut");
                }
                // エンティティ単位で排他ロック
                tbl.getLock(Header::WRITE_LOCK);
                // 更新可能なら更新ロックを自Trに更新
                entry.lock = trid;
                // エンティティ単位ロック開放
                tbl.releaseLock();
            }
            // 全体管理領域ロック解除
            Transaction::getTrans().releaseLock();
            // デフォルトマッチャなしまたはマッチの場合、結果に追加
            rows.push_back(rowid);
        }
    }
    // ソート
    if(sorter != nullptr && rows.size() != 0) {
        TRACE_LOG("[Sorter Execute] " << tblName << " begin:" << *(rows.begin()) <<" end:" << *(rows.end() - 1));
        std::sort(rows.begin(), rows.end(), SorterWapper(tbl, sorter));
    }

    return;
}

/**************************************************************************//**
*
*     関数名：データ挿入本体 (insert_tuple)
* <pre>
*
*    １    機能
*            指定したデータを対象のエンティティへ挿入する。本体
*
*    ２    引数
*            trid         : トランザクションID          [入力]
*            table_name   : エンティティ名              [入力]
*            data         : 挿入対象のデータ            [入力]
*            size         : データサイズ                [入力]
*
*    ３    戻り値
*            EXECUTE_OK       : 成功
*            EXECUTE_ERR      : 失敗
*            EXECUTE_NULL     : インデックス追加失敗
*            EXECUTE_FULL     : メモリ不足
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void IndexManager::insert_tuple(trid_t trid, const string& name,
        const AbstEntity& table, size_t size) {

    // 引数チェック
    if(trid == TRID_MAX) TRANSACTION_MISMATCH("トランザクションが開始されていません");

    Entity& tbl = Entity::getAddr(name);

    // サイズチェック
    if(size != tbl.tuple_size)
        LENGTH_ERROR("サイズ不一致 " << name << " (object=" << size
                << " entity=" << tbl.tuple_size << ")");

    // エンティティ単位で排他ロック
    tbl.getLock(Header::WRITE_LOCK);

    // テーブル挿入
    rowid_t rowid = tbl.createTuple(trid);
    tbl.releaseLock();

    // 挿入データをコピー
    tbl.setTuple(rowid, table);

    // インデックス管理情報有無チェック
    if(check_index(tbl)) {
        // インデックス挿入
        auto it = Initializer::index_map.find(tbl.getName());
        for(auto i = it->second.begin(); i != it->second.end(); i++) {
            rowid_t root = INVALID_ROWID;
            const string idxid = i->first;
            rowid_vec_t idxv;
            // インデックス管理情報検索
            IndexName tpl;
            load_index_root(trid, tbl, idxid, tpl);
            // インデックス管理インデックス領域取得
            Index& idx = Index::getAddr(tpl.index_name);
            // インデクサ取得
            const ImplIndexer& idxer = IndexerCache::getIndexer(tpl.indexer_name);
            // インデックス挿入
            root = idx.insertNode(trid, tpl.index_root, tbl, rowid, idxer);
            // 挿入後のインデックスルート保管
            store_index_root(trid, tbl, idx, idxid, tpl.indexer_name, root);
        }
    }   // インデックス系の処理はここまで

    SHM_DEBUG_DMP(INSERT, tbl.getName().c_str(), rowid,
            &tbl.getTuple(rowid), tbl.tuple_size);

    return;
}

/**************************************************************************//**
*
*     関数名：データ削除本体 (delete_tuples)
* <pre>
*
*    １    機能
*            指定した条件に沿ったデータを削除する
*
*    ２    引数
*            trid               : トランザクションID          [入力]
*            table_name         : エンティティ名              [入力]
*            index_id           : インデックスID              [入力]
*            index_matcher      : インデックスマッチャ        [入力]
*            default_matcher    : デフォルトマッチャ          [入力]
*
*    ３    戻り値
*            負の数    : 失敗
*            正の数    : 成功。値は条件にマッチしたデータ数(削除データ数)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void IndexManager::delete_tuples(trid_t trid, const string& tblName,
        AbstIndexMatcher* idxMtcr, const ImplMatcher* dftMtcr) {

    Entity& tbl = Entity::getAddr(tblName);

    // エンティティ本体削除対象検索(更新ロック付き)
    rowid_vec_t rows;
    search_tuples(rows, true, trid, tblName, idxMtcr, dftMtcr);

    // インデックス管理情報有無チェック
    if(check_index(tbl)) {
        // インデックス挿入
        auto it = Initializer::index_map.find(tbl.getName());
        for(auto i = it->second.begin(); i != it->second.end(); i++) {
            rowid_t root = INVALID_ROWID;
            const string& idxid = i->first;
            // インデックス管理情報検索
            IndexName tpl;
            load_index_root(trid, tbl, idxid, tpl);
            // インデックス管理インデックス領域取得
            Index& idx = Index::getAddr(tpl.index_name);
            // インデクサ取得
            const ImplIndexer& idxr = IndexerCache::getIndexer(tpl.indexer_name);
            root = tpl.index_root;
            for(auto j = rows.begin(); j != rows.end(); j++)
                root = idx.deleteNode(trid, root, tbl, *j, idxr);
            // 全体インデックス管理情報の更新
            store_index_root(trid, tbl, idx, idxid, tpl.indexer_name, root);
        }
    }

    // エンティティ本体の削除
    for(auto it = rows.begin(); it != rows.end(); it++) {
        rowid_t rowid = *it;
        if(rowid < 0) OUT_OF_RANGE("エンティティ領域が不正です");

        SHM_DEBUG_DMP(DELETE, tbl.getName().c_str(), rowid,
                &tbl.getTuple(rowid), tbl.tuple_size);
        tbl.deleteTuple(trid, rowid);
    }
    return;
}

/**************************************************************************//**
*
*     関数名：インデックス開始位置の取得 (load_index_root)
* <pre>
*
*    １    機能
*            インデックス開始位置の取得
*
*    ２    引数
*           trid                : トランザクションID            [入力]
*           entityp             : エンティティアドレス          [入力]
*           index_id            : インデックスID                [入力]
*           tuple               : タプル                        [入力]
*
*    ３    戻り値
*            EXECUTE_OK  : 成功
*            EXECUTE_ERR : 失敗
*
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void IndexManager::load_index_root(trid_t trid, Entity& ent,
        const string& idxid, IndexName& tpl) {

    IndexManager& idxMgr = IndexManager::getAddr();
    if(&idxMgr == &ent) {
        Transaction& trn = Transaction::getTrans();
        Transaction::Recode& tr = trn.getTransaction(trid);
        // 全体管理領域で共有ロックを取得する
        trn.getLock(Header::READ_LOCK);

        tpl.set(IndexName::ENTITY_NAME, IndexNameIndexer::INDEXER_NAME,
                IndexNameIndexer::INDEXER_NAME, IndexNameIndexer::INDEXER_NAME,
                tr.index_root);

        SHM_TRACE_LOG("TRN(trid:%lu) root:%ld", trid, tr.index_root);
        SHM_DEBUG_DMP(LOAD_TRN, trn.getName().c_str(), trid, &tr, sizeof(tr));

        // 全体管理領域ロック解除
        trn.releaseLock();
        return;
    }

    // TODO(指定されたIndexIDが存在するかを確認する)
    auto i = Initializer::index_map.find(ent.getName());
    if(i != Initializer::index_map.end() &&
            i->second.find(idxid) == i->second.end())
        NOT_DEFINE("インデックスIDがありません");
    // インデックス管理領域の検索
    IndexNameMatcher node_matcher(ent.getName(), idxid);
    rowid_vec_t idxs;
    search_tuples(idxs, false, trid, IndexName::ENTITY_NAME, &node_matcher);
    // TODO(正常に検索された場合1件のみ)
    if(idxs.size() == 1) {
        ::memcpy(&tpl, &idxMgr.getTuple(idxs.at(0)), idxs[0]);

        SHM_TRACE_LOG("IDX(trid:%lu rowid:%ld) root:%ld",
                trid, idxs[0], tpl.index_root);
        SHM_DEBUG_DMP(LOAD_IDX, idxMgr.getName().c_str(), idxs[0], &tpl,
                sizeof(tpl));
        return;
    }
    // 複数検索された場合はNG
    if(idxs.size() != 0) MULTI_DEFINE("複数のIndexが検索されました " << idxs.size());

    // TODO(初期状態の場合はindex_rootにINVALID_ROWIDを返却する)
    // (その他のパラメータはなくてもいい。というかわからない)
    auto j = Initializer::index_map.find(ent.getName())->second.find(idxid);
    tpl.set(ent.getName(), idxid, j->second.index_name,
            j->second.indexer_name, INVALID_ROWID);
    return;
}

/**************************************************************************//**
*
*     関数名：インデックス開始位置の保存 (store_index_root)
* <pre>
*
*    １    機能
*            インデックス開始位置の取得
*
*    ２    引数
*           trid                : トランザクションID            [入力]
*           entityp             : エンティティアドレス            [入力]
*           indexp              : インデクサドレス              [入力]
*           index_id            : インデックスID              [入力]
*           indexer_name        : インデクサ名                [入力]
*           index_root          : インデックス開始位置        [入力]
*
*    ３    戻り値
*            インデックスルートRowID
*            マイナス値 : 失敗
*
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void IndexManager::store_index_root(trid_t trid, Entity& ent, Index& idx,
        const string& idxid, const string& idxerName, rowid_t root) {

    Transaction& trn = Transaction::getTrans();
    Transaction::Recode& tr = trn.getTransaction(trid);
    IndexManager& idxMgr = IndexManager::getAddr();

    // エンティティがインデックス管理情報なら
    if(&idxMgr == &ent) {
        // 全体管理領域で排他ロックを取得する
        trn.getLock(Header::WRITE_LOCK);

        //トランザクション情報にindex_rootを格納する。
        SHM_TRACE_LOG("TRN(trid:%lu root:%ld -> TRNroot:%ld",
                trid, root, tr.index_root);

        tr.index_root = root;
        // TODO(ここでindex_root_masterを上書きしてはいけない)
        SHM_DEBUG_DMP(STORE_TRN, trn.getName().c_str(), trid, &tr, sizeof(tr));

        trn.releaseLock();

        return;
    }

    // エンティティ名-インデックスID検索用マッチャ定義
    IndexNameMatcher node_matcher(ent.getName(), idxid);
    // 対象のインデックス管理領域を削除
    delete_tuples(trid, IndexName::ENTITY_NAME, &node_matcher);
    // インデックス名称マスタの構成
    IndexName tpl(ent.getName(), idxid, idx.getName(), idxerName, root);

    // 新しいインデックス管理領域を追加
    insert_tuple(trid, IndexName::ENTITY_NAME, tpl, sizeof(tpl));

    SHM_TRACE_LOG("IDX(trid:%lu) root:%ld", trid, root);
    SHM_DEBUG_DMP(STORE_IDX, idxMgr.getName().c_str(), root, &tpl, sizeof(tpl));

    return;
}

/**************************************************************************//**
*
*     関数名：インデックスルートをロックする (lock_index_root)
* <pre>
*
*    １    機能
*            インデックス管理インデックスのルートインデックスを更新ロックする
*
*    ２    引数
*           *entity_name: ロック元のエンティティ名
*            trid : ロックしようとしているTrのTrID
*
*    ３    戻り値
*            ロックが採取できた場合trueを返す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
bool IndexManager::lock_index_root(const string& entName, trid_t trid) {
    // インデックス管理インデックスがまだ定義されていない場合は
    // ロック取得完了とする。
    if(Initializer::index_map.count(entName) == 0) return true;

    Transaction& trn = Transaction::getTrans();
    Index& idx = IndexManager::getIndex();

    bool ret = false;
    // 全体管理領域の共有ロックを取得
    trn.getLock(Header::READ_LOCK);
    // TODO(root_masterの無効チェックが必要)
    if(trn.index_root_master == INVALID_ROWID) {
        // 無効の場合はロックを採取しないで成功を返却する
        ret = true;
    } else {
        Entry& entry = idx.getEntry(trn.index_root_master);
        // インデックス管理インデックス領域の排他ロックを取得
        trn.getLock(Header::WRITE_LOCK);
        // 対象のindex_rootが読めて書き込める状態なら更新ロックを設定
        if(check_tuple_writable(trid, entry) != LOCKED) {
            entry.lock = trid;
            ret = true;
        }
        // 排他ロック開放
        trn.releaseLock();
    }
    // 共有ロック開放
    trn.releaseLock();

    return ret;
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}   // namespace SharedMemory
