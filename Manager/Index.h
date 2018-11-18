/**************************************************************************//**
* @file
*     モジュール名：個別インデックス管理情報クラスヘッダ
* <pre>
*
*    １  機能
*          個別インデックス管理情報クラス
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#ifndef SHAREDMEMORY_CSHAREDMEMORYINDEX_H_
#define SHAREDMEMORY_CSHAREDMEMORYINDEX_H_

#include <Entity/ImplMatcher.h>
#include <Manager/Entity.h>
#include <Manager/Transaction.h>

#include "inc/SHMConst.h"

namespace SharedMemory
{
/**************************************************************************//**
* クラス名 : 個別インデックス管理情報クラス(CSharedMemoryIndex)
*            個別のインデックス情報を管理する。
**//**************************************************************************/
class Index : public Entity {
private:
    /*----1---------2---------3---------4---------5---------6---------7---*//**
     * 構造体名 : 共通メモリ管理機能 個別インデックスノード情報定義(NODE)
    **//*-1---------2---------3---------4---------5---------6---------7------*/
    class IndexNode : public ::Entity::AbstEntity {
    public:
        ::Entity::rowid_t left;     ///< Nodeの左側のrowid
        ::Entity::rowid_t right;    ///< Nodeの右側のrowid
        ::Entity::rowid_t index;    ///< 対象エンティティのrowid
        int               priority; ///< ２分岐検索キーのプライオリティ(乱数)

        explicit IndexNode() : left(0), right(0), index(0), priority(0) { }
        virtual ~IndexNode() { };
    };

 public:
    /**********************************************************************//**
    *   関数名 : サイズ取得(getSize)
    *            フィールド数(LINE)をもとに必要なデータサイズを取得する。
    *   引数   : num : フィールド数(LINE)                              [入力]
    *   戻り値 : メモリサイズ(byte)
    **//*********************************************************************/
    static inline size_t getSize(size_t num) {
        return Entity::getSize(num, sizeof(Index::IndexNode));
    }

    /**********************************************************************//**
    *   関数名 : 個別インデックス情報アドレス取得 (getIndex)
    *            指定した個別インデックス情報のアドレスを得る
    *   引数   : index_name  : インデックス名     [入力]
    *   戻り値 : 個別インデックス情報の先頭アドレス(ポインタ)
    **//**********************************************************************/
    static inline Index& getAddr(const ::std::string& name) {
        return static_cast<Index&>(Entity::getAddr(name));
    }

    /**********************************************************************//**
    *   関数名 : 初期化(init)
    *            個別インデックス管理領域を初期化する。
    *   引数   : name   : 領域名                             [入力]
    *            num    : フィールド数(LINE)                 [入力]
    *   戻り値 : なし
    **//**********************************************************************/
    inline void init(const ::std::string& name, ::Entity::rowid_t num) {
        Entity::init(name, num, sizeof(Index::IndexNode));
    }

 private:
    /// ノード取得
    IndexNode& getNode(trid_t, ::Entity::rowid_t);
    /// タプル取得
    ::Entity::AbstEntity& getTarget(trid_t, ::Entity::rowid_t, Entity&);
    /// ノード右回転
    ::Entity::rowid_t rotate_right(trid_t, ::Entity::rowid_t);
    /// ノード左回転
    ::Entity::rowid_t rotate_left(trid_t, ::Entity::rowid_t);
    /// ノード検索
    ::Entity::rowid_t search_nodes(::Entity::rowid_vec_t&, const bool,
            const trid_t, const ::Entity::rowid_t,
            Entity&, const ::Entity::ImplMatcher* = nullptr,
            const ::Entity::ImplMatcher* = nullptr);
    /// ノード追加
    ::Entity::rowid_t insert_node(const trid_t, const ::Entity::rowid_t,
            Entity&, const ::Entity::rowid_t, const ::Entity::ImplIndexer&);
    /// ノード削除
    ::Entity::rowid_t delete_node(const trid_t, const ::Entity::rowid_t,
            Entity&, const ::Entity::rowid_t, const ::Entity::ImplIndexer&);
 public:
    /// ノード検索基底
    int searchNodes(::Entity::rowid_vec_t&, const bool, trid_t,
            const ::Entity::rowid_t, Entity&,
            const ::Entity::ImplMatcher*, const ::Entity::ImplMatcher*);
    /// ノード登録基底
    ::Entity::rowid_t insertNode(const trid_t, const ::Entity::rowid_t,
            Entity&, const ::Entity::rowid_t, const ::Entity::ImplIndexer&);
    /// ノード削除基底
    ::Entity::rowid_t deleteNode(const trid_t, const ::Entity::rowid_t,
            Entity&, const ::Entity::rowid_t, const ::Entity::ImplIndexer&);
};

/*--------1---------2---------3---------4---------5---------6---------7------*/
} // end namespace SharedMemory

#endif  // SHAREDMEMORY_CSHAREDMEMORYINDEX_H_
