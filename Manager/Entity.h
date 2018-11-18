/**************************************************************************//**
* @file
*     クラス名：共通メモリ管理機能 個別管理情報（個別エンティティ管理領域）
* <pre>
*
*    １  機能
*          個別管理情報（個別エンティティ管理領域）の構造を定義する
*          当クラスｋは、個別インデックス管理領域の基底クラスでもある
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/
#ifndef SHAREDMEMORY_ENTITY_H_
#define SHAREDMEMORY_ENTITY_H_

#include <Entity/AppTable.h>
#include <Manager/Header.h>
#include <Manager/Transaction.h>

#include <unistd.h>
#include <sys/timeb.h>

#include <map>
#include <string>

#include "inc/SHMConst.h"

namespace SharedMemory
{
/**************************************************************************//**
* クラス名：共通メモリ管理機能 個別管理情報（個別エンティティ管理領域）
*     (CSharedMemoryEntity)
*          個別管理情報（個別エンティティ管理領域）の構造を定義する
*          当クラスは、個別インデックス管理領域の基底クラスでもある
**//**************************************************************************/
class Entity : public Header {
public:
    size_t  tuple_size;             ///< キャッシュ要素サイズ
    ::Entity::rowid_t used_end;     ///< 使用中エントリ終端位置
                                    // ([0～used_end]の範囲にデータが存在する)
    ::Entity::rowid_t free_begin;   ///< 空きエントリ開始位置([free_begin～
                                    // tuple_num]の範囲に空きが存在する)
    /**********************************************************************//**
    * 構造体名：共通メモリ管理機能 個別データ管理情報定義(ENTRY)
    **//**********************************************************************/
    class Entry {
    public:
        trid_t xmin;                ///< 最小TRID(このID以上で可視)
        trid_t xmax;                ///< 最大TRID(このID未満で可視)
        trid_t lock;                ///< ロック取得TRID
    };

    Entry tag_entries[0];         ///< 個別管理領域配列

    /// テーブルステータス(enum)
    enum Status {
        WRITEABLE,      ///< 上書き可能
        INSERTABLE,     ///< 挿入が可能
        LOCKED          ///< 更新ロック中
    };

public:
    /**********************************************************************//**
    *    関数名 : 個別管理情報サイズ取得 (getSize)
    *             フィールド数、要素サイズから個別管理情報を確保するうえで
    *             必要なメモリサイズを取得する
    *    引数   : num       :    フィールド数(LINE)     [入力]
    *             unit_size :    データサイズ(byte)     [入力]
    *
    *    戻り値 : メモリサイズ(byte)
    **//**********************************************************************/
    static inline size_t getSize(size_t num, size_t unit_size) {
        return sizeof(Entity) + (sizeof(Entry) + unit_size) * num;
    }

    /// 個別エンティティ情報アドレス取得
    static Entity& getAddr(const std::string&);
    /// 領域可視判定
    static bool check_tuple_readable(trid_t, Entity::Entry&);
    /// 領域操作可否判定
    static Status check_tuple_writable(trid_t, Entity::Entry&);
    /// ROW識別子チェック
    void checkRowID(::Entity::rowid_t);
    /// 個別データ管理情報アドレス取得
    Entry& getEntry(::Entity::rowid_t);
    /// 個別データ本体取得
    ::Entity::AbstEntity& getTuple(::Entity::rowid_t);
    /// 個別データ本体設定
    void setTuple(::Entity::rowid_t, const ::Entity::AbstEntity&);
    /// 初期化
    void init(const ::std::string&, const ::Entity::rowid_t, const size_t);
    /// データフィールド作成
    ::Entity::rowid_t createTuple(trid_t);
    /// データフィールド更新
    ::Entity::rowid_t updateTuple(trid_t, ::Entity::rowid_t);
    /// データフィールド論理削除
    int  deleteTuple(trid_t, ::Entity::rowid_t);
    /// データフィールド物理削除
    void freeTuple(::Entity::rowid_t);
};

/*--------1---------2---------3---------4---------5---------6---------7------*/
}   // end namespace ShahedMemory

#endif /* SHAREDMEMORY_ENTITY_H_ */
