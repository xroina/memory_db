/**************************************************************************//**
* @file
*     モジュール名：個別インデックス管理情報クラス
* <pre>
*
*    １  機能
*          個別インデックス管理情報クラス
*
*    ２  関数名一覧
*           サイズ取得               (getSize)
*           初期化                   (init)
*           ノードアドレス取得       (getNodeAddr)
*           対象タプルアドレス取得   (getTargetAddr)
*           ノード右回転             (rotate_right)
*           ノード左回転             (rotate_left)
*           ノード検索               (search_nodes)
*           ノード追加               (insert_node)
*           ノード削除               (delete_node)
*
*    ３  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#include <Manager/Index.h>
#include <Manager/Transaction.h>
#include <cstdlib>

#include "inc/SHMConst.h"
#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::Entity::AbstEntity;
using ::Entity::rowid_t;
using ::Entity::INVALID_ROWID;
using ::Entity::rowid_vec_t;
using ::Entity::ImplMatcher;
using ::Entity::ImplSorter;
using ::Entity::ImplIndexer;

/**************************************************************************//**
*
*     関数名： ノードアドレス取得(getNodeAddr)
* <pre>
*
*    １    機能
*            フィールド番号から、対象の共有データの実体アドレスを取得する
*
*    ２    引数
*            self_trid  : 自トランザクションID          [入力]
*            rowid      : 要素番号                      [入力]
*
*    ３    戻り値
*            共有データ実体のアドレス(ポインタ)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
Index::IndexNode& Index::getNode(trid_t trid, rowid_t rowid) {
    // 参照可否チェック
    if(!check_tuple_readable(trid, getEntry(rowid)))
        OUT_OF_RANGE("対象インデックスが参照できません"
                "(name:" << getName() << " RowID:" << rowid <<
                " MaxLine:" << getMaxLine() << ")");
    return static_cast<Index::IndexNode&>(getTuple(rowid));
}

/**************************************************************************//**
*
*     関数名： 対象タプルアドレス取得(getTargetAddr)
* <pre>
*
*    １    機能
*            トランザクションID、要素番号から、ノード指す対象タプルアドレス
*            を取得する。
*
*    ２    引数
*            self_trid  : 自トランザクションID          [入力]
*            rowid      : 要素番号                      [入力]
*            table      : テーブルエンティティ[入力]
*
*    ３    戻り値
*            対象タプルのアドレス(ポインタ)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
AbstEntity& Index::getTarget(trid_t trid, rowid_t rowid, Entity& tbl) {

    return tbl.getTuple(getNode(trid, rowid).index);
}

/**************************************************************************//**
*
*     関数名： ノード右回転(rotate_right)
* <pre>
*
*    １    機能
*            二分木を右回転し、親ノードと子ノード入れ替える。
*
*    ２    引数
*            self_trid  : 自トランザクションID          [入力]
*            ctx_node   : 移動対象の要素番号            [入力]
*
*    ３    戻り値
*            回転後のルートノード
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::rotate_right(trid_t trid, rowid_t node) {
    // 親ノードのコピーとして新右ノード作成
    rowid_t right = updateTuple(trid, node);
    if(right < 0) return right;
    // 新右ノードアドレス取得
    IndexNode& right_node = getNode(trid, right);

    // 元ルートの左ノードを回転後のルートノードに設定
    // 元親ルートの左ノードをコピーして新親ノード作成
    rowid_t left = updateTuple(trid, right_node.left);
    if(left < 0) return left;
    // 新親(左)ノードアドレス取得
    IndexNode& left_node = getNode(trid, left);

    // 新しいルートノードの右ノードを元のノードの左ノードに設定。
    // 新親(左)ノードの右ノードを旧親(新右)ノードの左ノードに設定
    right_node.left = left_node.right;
    // 新親(左)ノードの右ノードを旧親(新右)ノードに設定
    left_node.right = right;
    // 新親(左)ノードを新しい親ノードとして返す。

    return left;
}

/**************************************************************************//**
*
*     関数名： ノード左回転(rotate_left)
* <pre>
*
*    １    機能
*            二分木を左回転し、親ノードと子ノード入れ替える。
*
*    ２    引数
*            self_trid  : 自トランザクションID          [入力]
*            ctx_node   : 移動対象の要素番号            [入力]
*
*    ３    戻り値
*            回転後のルートノード
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::rotate_left(trid_t trid, rowid_t node) {
    // 親ノードのコピーとして新左ノード作成
    rowid_t left = updateTuple(trid, node);
    if(left < 0) return left;
    // 新左ノードアドレス取得
    IndexNode& left_node = getNode(trid, left);

    // 元のルートの右ノードを回転後のルートノードに設定。
    // 元親ルートの右ノードをコピーして新親ノード作成
    rowid_t right = updateTuple(trid, left_node.right);
    if(right < 0) return left;
    // 新親(右)ノードアドレス取得
    IndexNode& right_node = getNode(trid, right);

    // 新しいルートノードの左ノードを元のノードの右ノードに設定。
    // 新親(右)ノードの左ノードを旧親(新左)ノードの左ノードに設定
    left_node.right = right_node.left;
    // 新親(右)ノードの左ノードを旧親(新左)ノードに設定
    right_node.left = left;
    // 新親(右)ノードを新しい親ノードとして返す。

    return right;
}

/**************************************************************************//**
*
*     関数名： ノード検索(search_nodes)
* <pre>
*
*    １    機能
*            ノードを検索する
*
*    ２    引数
*            rowv     : 検索結果RowID格納用vector     [出力]
*            flag     : 更新ロックフラグ              [入力]
*            trid     : 自トランザクションID          [入力]
*            rowid    : 要素番号                      [入力]
*            tbl      : テーブルエンティティ          [入力]
*            idxMtcr  : インデックスマッチャ          [入力]
*            dftMtcr  : デフォルトマッチャ            [入力]
*
*    ３    戻り値
*            EXECUTE_OK        : 正常終了
*            EXECUTE_ERR       : 異常終了
*            EXECUTE_FULL      : メモリが足りない？
*            EXECUTE_TIMEOUT   : 上位でタイムアウトに倒す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::search_nodes(rowid_vec_t& rows, const bool flag, const trid_t trid,
        const rowid_t cNode, Entity& tbl, const ImplMatcher* idxMtcr,
        const ImplMatcher* dftMtcr) {
    rowid_t ret = INVALID_ROWID;
    // 引数チェック
    if(cNode == INVALID_ROWID) return EXECUTE_OK;
    if(cNode < 0) return cNode;
    // インデックスノード取得
    const IndexNode& node = getNode(trid, cNode);

    // エンティティ本体取得
    AbstEntity& data = getTarget(trid, cNode, tbl);
    // インデックスマッチャ実行
    int i = 0;
    if(idxMtcr != nullptr) i = idxMtcr->match(data);

    // 一致または大きい場合、左側探索（後続処理も行う）
    if(i >= 0) {
        ret = search_nodes(rows, flag, trid, node.left, tbl, idxMtcr, dftMtcr);
        if(ret < 0 && ret != INVALID_ROWID) return ret;
    }
    // 一致の場合、デフォルトマッチャ実行（後続処理も行う）
    if(i == 0) {
        if(dftMtcr == nullptr || 0 == dftMtcr->match(getTarget(trid, cNode, tbl))) {
            // 一致した場合は格納する。
            rowid_t rowid = node.index;

            // 更新ロックフラグONなら
            if(flag) {
                // 対象エントリの更新ロック取得可否を調べる
                Entry& ent = tbl.getEntry(rowid);
                if(check_tuple_writable(trid, ent) != LOCKED) {
                    // 更新できるならロックを取得する。
                    ent.lock = trid;
                } else {
                    // 更新できないなら上位でタイムアウトに倒す
                    return EXECUTE_TIMEOUT;
                }
            }
            // RowIDをvecterに登録
            rows.push_back(rowid);
        }
    }
    // 一致または小さい場合、右側探索
    if(i <= 0) {
        ret = search_nodes(rows, flag, trid, node.right, tbl, idxMtcr, dftMtcr);
        if(ret < 0 && ret != INVALID_ROWID) return ret;
    }
    return EXECUTE_OK;
}

/**************************************************************************//**
*
*     関数名： ノード登録(insert_node)
* <pre>
*
*    １    機能
*            ノードを登録する。
*
*    ２    引数
*            self_trid  : 自トランザクションID           [入力]
*            ctx_node   : 起点となるノード               [入力]
*            table      : テーブルエンティティ           [入力]
*            rowid      : 登録するノード                 [出力]
*            indexer    : インデクサ                     [入力]
*
*    ３    戻り値
*            プラスの数        : 登録されたノード数
*            EXECUTE_ERR       : 異常終了
*            EXECUTE_FULL      : メモリが足りない
*            EXECUTE_NULL      : どこかでNULLがあったり無効だったりした
*            EXECUTE_TIMEOUT   : 上位でタイムアウトに倒す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::insert_node(const trid_t trid, const rowid_t cNode, Entity& tbl,
        const rowid_t rowid, const ImplIndexer& idxr) {
    // 引数チェック
    if(rowid < 0) INVALID_ARGUMENT("RowIDが無効です");

    if(cNode < 0 || cNode == INVALID_ROWID) {
        // インデックス単位で排他ロック
        getLock(Header::WRITE_LOCK);

        // インデックスがない場合、新しいノードを作成
        rowid_t new_node = createTuple(trid);
        if(new_node >= 0) {
            IndexNode& node = getNode(trid, new_node);
            node.left = INVALID_ROWID;
            node.right = INVALID_ROWID;
            node.index = rowid;
            node.priority = ::random();

            SHM_DEBUG_DMP(INS_NODE, getName().c_str(), new_node, &node, sizeof(node));
        }
        releaseLock();

        return new_node;
    }
    // インデクサで値を比較
    int i = idxr.compare(getTarget(trid, cNode, tbl), tbl.getTuple(rowid));
    if(i == 0) {
        // 同一の値が存在する場合
        SHM_WARN_LOG("同じタプルが指定されています。");
        return EXECUTE_KEYERR;
    }
    // いまのノードをコピーして新しいノードを作る
    rowid_t newNode = updateTuple(trid, cNode);
    if(newNode < 0) return newNode;
    IndexNode& node = getNode(trid, newNode);

    if(i > 0) {
        // ノードの方が大きい場合、ノードの左側に挿入する(再起呼出)
        rowid_t left = insert_node(trid, node.left, tbl, rowid, idxr);
        if (left < 0) return left;
        IndexNode& left_node = getNode(trid, left);

        node.left = left;
        // 優先度の大小が逆転している場合は、右回転を行う。
        if (node.priority > left_node.priority)
            newNode = this->rotate_right(trid, newNode);
    } else {
        // ノードの方が小さい場合、ノードの右側に挿入する(再起呼出)
        rowid_t right = insert_node(trid, node.right, tbl, rowid, idxr);
        if (right < 0) return right;
        IndexNode& right_node = getNode(trid, right);

        node.right = right;
        // 優先度の大小が逆転している場合は、左回転を行う。
        if (node.priority > right_node.priority)
            newNode = this->rotate_left(trid, newNode);
    }
    SHM_DEBUG_DMP(INS_NODE, getName().c_str(), newNode, &node, sizeof(node));
    // 新しいノードを返却
    return newNode;
}

/**************************************************************************//**
*
*     関数名： ノード削除(delete_node)
* <pre>
*
*    １    機能
*            ノードを削除する。
*
*    ２    引数
*            self_trid  : 自トランザクションID           [入力]
*            ctx_node   : 起点となるノード               [入力]
*            table      : テーブルエンティティ           [入力]
*            indexer    : インデクサ                     [入力]
*            rowid      : 削除するノード                 [出力]
*
*    ３    戻り値
*            プラスの数        : 削除後の新しいノード
*            INVALID_ROWID     : ノードの末端に達した
*            EXECUTE_FULL      : メモリが足りない
*            EXECUTE_NULL      : どこかでNULLがあったり無効だったりした
*            EXECUTE_TIMEOUT   : 上位でタイムアウトに倒す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::delete_node(const trid_t trid, const rowid_t cNode, Entity& tbl,
        const rowid_t rowid, const ImplIndexer& idxr) {
    // 引数チェック
    if(rowid < 0) INVALID_ARGUMENT("RowIDが無効です");

    if(cNode < 0) return cNode;

    IndexNode& node = getNode(trid, cNode);

    // インデクサで比較する。
    int i = idxr.compare(getTarget(trid, cNode, tbl), tbl.getTuple(rowid));

    rowid_t newNode = INVALID_ROWID;
    if(i == 0) {
        SHM_DEBUG_DMP(DEL_NODE, getName().c_str(), rowid, &node, sizeof(node));
        // 一致した場合
        if(node.left < 0 && node.right < 0) {
            // 左右の子ノードが存在しない場合、削除する。
            newNode = deleteTuple(trid, cNode);
            if (newNode < 0) return newNode;
            return INVALID_ROWID;
        } else if(node.left < 0) {
            // 左ノードが存在しない場合、左回転して末端に移動する。
            newNode = rotate_left(trid, cNode);

        } else if(node.right < 0) {
            // 右ノードが存在しない場合、右回転して末端に移動する。
            newNode = rotate_right(trid, cNode);
        } else {
            // 両ノードが存在する場合、優先度に応じて回転する。
            IndexNode& left_node  = getNode(trid, node.left);
            IndexNode& right_node = getNode(trid, node.right);
            if(left_node.priority < right_node.priority) {
                newNode = rotate_right(trid, cNode);
            } else {
                newNode = rotate_left(trid, cNode);
            }
        }
        // 削除する。
        newNode = delete_node(trid, newNode, tbl, rowid, idxr);
    } else {
        // 一致しない場合は自NODEをコピーして新しいノードを作る
        newNode = updateTuple(trid, cNode);
        if(newNode < 0) return newNode;
        IndexNode& node = getNode(trid, newNode);
        if(i > 0) {
            // 削除ノードが大きい場合、親ノードの左側に削除後のノードを
            // 再設定する(再起呼出)
            node.left = delete_node(trid, node.left, tbl, rowid, idxr);
        } else {
            // 削除ノードが小さい場合、親ノードの右側に削除後のノードを
            // 再設定する(再起呼出)
            node.right = delete_node(trid, node.right, tbl, rowid, idxr);
        }
    }
    return newNode;
}
/**************************************************************************//**
*
*     関数名： ノード検索基底(searchNodes)
* <pre>
*
*    １    機能
*            privateメソッドで再帰呼び出しを行うための基底メソッド
*
*    ２    引数
*            p_rowid_vec    : 検索結果RowID格納用vector     [出力]
*            lock_flag      : 更新ロックフラグ              [入力]
*            self_trid      : 自トランザクションID          [入力]
*            rowid          : 要素番号                      [入力]
*            table          : テーブルエンティティ          [入力]
*            index_matcher  : インデックスマッチャ          [入力]
*            default_matcher: デフォルトマッチャ            [入力]
*
*    ３    戻り値
*            EXECUTE_OK     : 正常終了
*            EXECUTE_ERR    : 異常終了
*            EXECUTE_FULL   : メモリが足りない？
*            EXECUTE_TIMEOUT: 上位でタイムアウトに倒す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
int Index::searchNodes(rowid_vec_t& rows, const bool lockFlag,
        const trid_t trid, const rowid_t root, Entity& tbl,
        const ImplMatcher* idxMtcr, const ImplMatcher* dftMtcr) {
    // 全体領域の共有ロック取得
    Transaction::getTrans().getLock(Header::READ_LOCK);
    int ret = search_nodes(rows, lockFlag, trid, root, tbl, idxMtcr, dftMtcr);
    Transaction::getTrans().releaseLock();
    return ret;
}

/**************************************************************************//**
*
*     関数名： ノード登録基底(insertNode)
* <pre>
*
*    １    機能
*            privateメソッドで再帰呼び出しを行うための基底メソッド
*
*    ２    引数
*            self_trid  : 自トランザクションID           [入力]
*            ctx_node   : 起点となるノード               [入力]
*            table      : テーブルエンティティ           [入力]
*            indexer    : インデクサ                     [入力]
*            rowid      : 登録するノード                 [入力]
*
*    ３    戻り値
*            プラスの数         : 登録されたノード数
*            EXECUTE_ERR       : 異常終了
*            EXECUTE_FULL      : メモリが足りない？
*            EXECUTE_TIMEOUT   : 上位でタイムアウトに倒す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::insertNode(const trid_t trid, const rowid_t root, Entity& tbl,
        const rowid_t rowid, const ImplIndexer& idxr) {
    // 全体領域の共有ロック取得
    Transaction::getTrans().getLock(Header::READ_LOCK);
    rowid_t ret = insert_node(trid, root, tbl, rowid, idxr);
    Transaction::getTrans().releaseLock();
    return ret;
}

/**************************************************************************//**
*
*     関数名： ノード削除基底(deleteNode)
* <pre>
*
*    １    機能
*            privateメソッドで再帰呼び出しを行うための基底メソッド
*
*    ２    引数
*            self_trid  : 自トランザクションID           [入力]
*            ctx_node   : 起点となるノード               [入力]
*            table      : テーブルエンティティ           [入力]
*            indexer    : インデクサ                     [入力]
*            rowid      : 削除するノード                 [出力]
*
*    ３    戻り値
*            プラスの数         : 登録されたノード数
*            EXECUTE_ERR       : 異常終了
*            EXECUTE_FULL      : メモリが足りない？
*            EXECUTE_TIMEOUT   : 上位でタイムアウトに倒す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*************************************************************************/
rowid_t Index::deleteNode(const trid_t trid, const rowid_t root, Entity& tbl,
        const rowid_t rowid, const ImplIndexer& idxr) {
    // 全体領域の共有ロック取得
    Transaction::getTrans().getLock(Header::READ_LOCK);
    rowid_t ret = delete_node(trid, root, tbl, rowid, idxr);
    Transaction::getTrans().releaseLock();
    return ret;
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}    // SharedMemory
