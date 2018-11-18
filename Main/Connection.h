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

#ifndef SharedMemory_CONNECTION_H_
#define SharedMemory_CONNECTION_H_

#include <Manager/Transaction.h>
#include <Entity/ImplMatcher.h>
#include <cstdlib>
#include <vector>

#include "PBase"
#include "inc/SHMConst.h"

namespace SharedMemory
{

class Cursor;                        ///< カーソルクラス

typedef ::std::vector<Cursor*> cursor_t;   ///< カーソル配列型
/**************************************************************************//**
*
*     クラス名：共通メモリ管理機能コネクションクラス (CSMConnection)
* <pre>
*
*    １    機能
*          共通メモリ管理機能コネクションクラスの外部インターフェース
*
*    ２    履歴
*            rev001 : 新規作成
* </pre>
**//**************************************************************************/
class Connection {
public:
    enum IsolationLevel {
        READ_COMMITTED, ///< リードコミッティッド
        SERIALIZABLE    ///< シリアライザブル
    };

private:
    trid_t trid;                ///< オブジェクトが持つトランザクションID
    IsolationLevel level;    ///< アイソレーションレベル
    cursor_t  cursor_vct;       ///< カーソルオブジェクト配列(vector)

public:
    explicit Connection();
    virtual ~Connection();

    /// カーソルオープン
    Cursor& openCursor(::Entity::AppTable&, const bool,
            ::Entity::AbstIndexMatcher* = nullptr,
            const ::Entity::ImplMatcher* = nullptr,
            const ::Entity::ImplSorter*  = nullptr);
    /// 挿入
    int executeInsert(::Entity::AppTable&);
    /// 更新
    int executeUpdate(::Entity::AppTable&,
            ::Entity::AbstIndexMatcher* = nullptr,
            const ::Entity::ImplMatcher* = nullptr);
    /// 削除
    int executeDelete(const ::std::string&,
            ::Entity::AbstIndexMatcher* = nullptr,
            const ::Entity::ImplMatcher* = nullptr);
    void commitTransaction();       ///< コミット
    void rollbackTransaction();     ///< ロールバック
    void close();                   ///< クローズ
    /// トランザクション分離レベル設定
    void setIsolationLevel(const IsolationLevel);
    /// トランザクション分離レベル取得
    const IsolationLevel getIsolationLevel() const;

    /**********************************************************************//**
    *
    *     関数名：コネクショントランザクションID取得(getTrID)
    * <pre>
    *
    *    １    機能
    *           コネクションが保持しているTrIDを取得する
    *
    *    ２    引数
    *            なし
    *
    *    ３    戻り値
    *            TrID
    *
    *    ４    履歴
    *            REV001 : 新規作成
    * </pre>
    **//**********************************************************************/
    inline trid_t getTrID() {
        return this->trid;
    }
    /**********************************************************************//**
    *
    *     関数名：コネクション所属カーソル取得(getCursorVector)
    * <pre>
    *
    *    １    機能
    *           コネクションが保持しているカーソルをvector配列として取得する
    *
    *    ２    引数
    *            なし
    *
    *    ３    戻り値
    *            カーソル配列(vector)
    *
    *    ４    履歴
    *            REV001 : 新規作成
    * </pre>
    **//**********************************************************************/
    inline cursor_t* getCursorVector() {
        return &this->cursor_vct;
    }

    /// トランザクション取得
    void getTransaction();
    /// トランザクション調整
    void adjustTransaction();
    /// 現在時刻取得(msec)
    static msec_t msecGet();
    /// タイムアウトチェック
    static bool timeCheck(msec_t);

    static void sleep();
};
/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // namespace SharedMemory

#endif // SharedMemory_CONNECTION_H_
