/**************************************************************************//**
* @file
*     モジュール名：インデックス管理情報クラスヘッダ
* <pre>
*          インデックス管理情報クラス
* </pre>
**//**************************************************************************/
#ifndef SHAREDMEMORY_INDEXMANAGER_H_
#define SHAREDMEMORY_INDEXMANAGER_H_

#include <Entity/ImplMatcher.h>
#include <Entity/IndexName.h>
#include <Init/Initializer.h>
#include <Manager/Entity.h>
#include <Manager/Header.h>
#include <Manager/Index.h>
#include <cstring>
#include <string>
#include <functional>

#include "inc/SHMConst.h"
#include "inc/SHMmacro.h"

namespace SharedMemory {
/**************************************************************************//**
* クラス名 : インデックス管理インデックス情報クラス(CIndexManager)
*            インデックス管理インデックス情報を管理する。
**//**************************************************************************/
class IndexManager : public Entity {

public:

    /**********************************************************************//**
    *   関数名 : インデックス名称マスタ管理情報アドレス取得(getIndexMapAddr)
    *            rowidが指し示すインデックス名称マスタ管理情報アドレスを
    *            取得する
    *   引数   : rowid   :   フィールドNo(LINE)     [入力]
    *   戻り値 : インデックス名称マスタ管理情報実体のアドレス(ポインタ)
    **//**********************************************************************/
    inline const ::Entity::IndexName& getIndexMapAddr(::Entity::rowid_t rowid) {
        return static_cast<const ::Entity::IndexName&>(this->getTuple(rowid));
    }

    /**********************************************************************//**
    *   関数名 : サイズ取得(getSize)
    *            フィールド数(LINE)をもとに必要なデータサイズを取得する。
    *   引数   : num : フィールド数(LINE)                          [入力]
    *
    *   戻り値 : メモリサイズ(byte)
    **//*********************************************************************/
    static inline size_t getSize(size_t num) {
        return Entity::getSize(num, sizeof(::Entity::IndexName));
    }

    /**********************************************************************//**
    *   関数名 : 全体インデックス管理インデックス情報アドレス取得 (getIndex)
    *            全体インデックス管理インデックス情報の先頭アドレスを取得する
    *   引数   : なし
    *   戻り値 : 先頭アドレスのポインタ
    **//**********************************************************************/
    static inline Index& getIndex() {
        if(nullptr == Initializer::index_index_addr)
            NULLPOINTER("インデックス管理インデックスが初期化されていません。");

        return *Initializer::index_index_addr;
    }

     /**********************************************************************//**
    *   関数名 : 全体インデックス管理情報アドレス取得 (getAddr)
    *            全体インデックス管理情報の先頭アドレスを取得する
    *   引数   : なし
    *   戻り値 : 先頭アドレスのポインタ
    **//**********************************************************************/
    static inline IndexManager& getAddr() {
        if(nullptr == Initializer::index_addr)
            NULLPOINTER("インデックス管理情報が初期化されていません。");

        return *Initializer::index_addr;
    }

    /**********************************************************************//**
    *   関数名 : 初期化(init)
    *            個別インデックス管理領域を初期化する。
    *   引数   : name   :   領域名                             [入力]
    *            file   :   ファイル名                         [入力]
    *            num    :   フィールド数(LINE)                 [入力]
    *
    *   戻り値 : なし
    **//***********************************************************************/
    inline void init(const ::std::string& name, const ::Entity::rowid_t num) {
        Entity::init(name, num, sizeof(::Entity::IndexName));
    }

    /// 検索
    static void search_tuples(::Entity::rowid_vec_t&, const bool,
            const trid_t, const ::std::string&,
            ::Entity::AbstIndexMatcher* = nullptr,
            const ::Entity::ImplMatcher* = nullptr,
            const ::Entity::ImplSorter* = nullptr);
    /// 登録
    static void insert_tuple(trid_t, const ::std::string&,
            const ::Entity::AbstEntity&, size_t size);
    /// 削除
    static void delete_tuples(trid_t, const ::std::string&,
            ::Entity::AbstIndexMatcher* = nullptr,
            const ::Entity::ImplMatcher* = nullptr);
public:
    /// インデックスルート更新ロック
    static bool lock_index_root(const ::std::string&, trid_t);

    /**********************************************************************//**
    *   関数名 : インデックスルート参照確認(is_index_root_valid)
    *            指定したTrIDとRowIDのインデックス管理インデックスが
    *            参照可能かを確認する。
    *   引数   : trid    : トランザクションID(TrID)
    *            root    : インデックスルート(RowID)
    *   戻り値 : true    : 参照可能
    *            false   : 参照不可能
    **//**********************************************************************/
    static inline bool is_index_root_valid(trid_t trid, ::Entity::rowid_t root) {
        if(nullptr == Initializer::index_index_addr) return false;
        if(::Entity::INVALID_ROWID == root) return false;
        if(Entity::check_tuple_readable(trid, getIndex().getEntry(root))) return true;
        return false;
    }

private:
    /// インデックスルート取得
    static void load_index_root(trid_t, Entity&,
            const ::std::string&, ::Entity::IndexName&);
    /// インデックスルート保管
    static void store_index_root(trid_t, Entity&,
            Index&, const ::std::string&, const ::std::string&, ::Entity::rowid_t);
private:
    /**********************************************************************//**
    *    関数名 : エンティティにインデックスが紐づいているか確認(check_index)
    *             エンティティにインデックスが紐づいているか確認する。
    *    引数   : ent     : エンティティのポインタ
    *    戻り値 : true    : あり
    *             false   : なし
    **//**********************************************************************/
    static inline bool check_index(Entity& ent) {
        if(Initializer::index_map.find(ent.getName()) !=
                Initializer::index_map.end()) return true;
        return false;
    }
private:
    /**********************************************************************//**
    * クラス名 : ソーターラッパー (sorter_wrapper)
    *            検索結果のソートを行うためのクラス
    **//**********************************************************************/
    class SorterWapper : public ::std::binary_function<::Entity::rowid_t, ::Entity::rowid_t, bool> {
    private:
        Entity& table;     ///< エンティティ
        const ::Entity::ImplSorter* sorter;   ///< ソーター
    public:
        /******************************************************************//**
        *
        *     関数名：ソーターラッパー (CSorterWapper)
        * <pre>
        *
        *    １    機能
        *            ソーターラッパーのコンストラクタ
        *
        *    ２    引数
        *           table           : エンティティ名             [入力]
        *           sorter          : ソーター                   [入力]
        *
        *    ３    戻り値
        *            なし
        *
        *    ４    履歴
        *            REV001 : 新規作成
        * </pre>
        **//******************************************************************/
        explicit SorterWapper(Entity& tbl, const ::Entity::ImplSorter* sorter) :
            table(tbl), sorter(sorter) { }

        /******************************************************************//**
        *
        *     関数名：ソーターラッパーオペレータ (operator)
        * <pre>
        *
        *    １    機能
        *            ソータそのままではstd::sortが使えないため、std::sortの仕様に
        *            沿った戻り値を返すためのメソッド
        *
        *    ２    引数
        *            rowid1 : 比較対象１のフィールドNo
        *            rowid2 : 比較対象２のフィールドNo
        *
        *    ３    戻り値
        *            rowid1 < rowid2 の関係のときに、trueを返す
        *
        *    ４    履歴
        *            REV001 : 新規作成
        * </pre>
        **//******************************************************************/
        bool operator()(const ::Entity::rowid_t& r1, const ::Entity::rowid_t& r2) const {
            if(sorter == nullptr) return false;
            return sorter->compare(table.getTuple(r1), table.getTuple(r2)) < 0;
        }

    };
};
/*--------1---------2---------3---------4---------5---------6---------7------*/
} // end namespace SharedMemory

#endif  // SHAREDMEMORY_INDEXMANAGER_H_
