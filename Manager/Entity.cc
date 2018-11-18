/**************************************************************************//**
* @file
*     クラス名：共通メモリ管理機能 個別管理情報（個別エンティティ管理領域）
*     (CSharedMemoryEntity)
* <pre>
*
*    １    機能
*          個別管理情報（個別エンティティ管理領域）の構造を定義する
*          当クラスｋは、個別インデックス管理領域の基底クラスでもある
*
*    ２    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
#include <Init/Initializer.h>
#include <Main/Access.h>
#include <Manager/Entity.h>
#include <Manager/Transaction.h>
#include <string.h>

#include <string>

#include "inc/SHMConst.h"
#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::std::string;
using ::Entity::rowid_t;
using ::Entity::AbstEntity;
/**************************************************************************//**
*
*     関数名：個別エンティティ情報アドレス取得 (getAddr)
* <pre>
*
*    １    機能
*            指定した個別エンティティ情報のアドレスを得る
*
*    ２    引数
*            name  :    エンティティ名     [入力]
*
*    ３    戻り値
*            個別エンティティ情報の先頭アドレス(ポインタ)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Entity& Entity::getAddr(const string& name) {

    auto it = Initializer::table_map.find(name);
    if(it == Initializer::table_map.end())
        INVALID_ARGUMENT("テーブルが存在しません:" << name);
    if(nullptr == it->second) {
        NULLPOINTER("テーブル" << name << "情報が初期化されていません")
    }
    return *const_cast<Entity*>(it->second);
}

void Entity::checkRowID(rowid_t rowid) {
    if(rowid < 0 || rowid > used_end || rowid >= static_cast<rowid_t>(getMaxLine()))
        OUT_OF_RANGE("RowIDが管理サイズを超えています:"
                "(Entity=" << getName() << " RowID=" << rowid <<
                " UsedEnd=" << used_end << " MaxLine="<< getMaxLine()<< ")");
}

/**************************************************************************//**
*
*     関数名：共有データ管理情報取得 (getEntry)
* <pre>
*
*    １    機能
*            フィールド番号から、対象の共有データ管理情報を取得する
*
*    ２    引数
*            rowid     :    フィールドNo(LINE)     [入力]
*
*    ３    戻り値
*            共有データ管理情報のアドレス
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Entity::Entry& Entity::getEntry(rowid_t rowid) {
    checkRowID(rowid);
    return tag_entries[rowid];
}

/**************************************************************************//**
*
*     関数名：共有データ実体アドレス取得 (getTupleAddr)
* <pre>
*
*    １    機能
*            フィールド番号から、対象の共有データの実体アドレスを取得する
*
*    ２    引数
*            rowid     :    フィールドNo(LINE)     [入力]
*
*    ３    戻り値
*            共有データ実体のアドレス(ポインタ)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
AbstEntity& Entity::getTuple(rowid_t rowid) {
    checkRowID(rowid);
    return *reinterpret_cast<AbstEntity*>(reinterpret_cast<size_t>(
            &tag_entries[getMaxLine()]) + (getUnitSize() * rowid));
}

void Entity::setTuple(rowid_t rowid, const AbstEntity& ent) {
    checkRowID(rowid);
    ::memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(
            &tag_entries[getMaxLine()]) + (getUnitSize() * rowid)),
            &ent, getUnitSize());
}
/**************************************************************************//**
*
*     関数名：個別管理領域初期化 (init)
* <pre>
*
*    １    機能
*            個別管理領域を初期化する
*
*    ２    引数
*            name      :    領域名                 [入力]
*            num       :    フィールド数(LINE)     [入力]
*            unit_size :    データサイズ(byte)     [入力]
*
*    ３    戻り値
*            なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Entity::init(const string& name, const rowid_t num, const size_t size) {
    Header::init(name, 0, num, getSize(num, size), size);
    tuple_size = size;
    free_begin = 0;
    // 対象全フィールドのxminを無効に更新する
    used_end = num;
    for(rowid_t rowid = 0; rowid < num; rowid++)
        getEntry(rowid).xmin = TRID_MAX;
    used_end = 0;
}

/**************************************************************************//**
*
*     関数名：要素読込可否チェック (check_tuple_readable)
* <pre>
*
*    １    機能
*            指定アドレスへの要素が読込み可能かどうかを、xmax/xminから判定する
*
*    ２    引数
*            self_trid  :    自トランザクションID          [入力]
*          * entry      :    対象要素のアドレスポインタ    [入力]
*
*    ３    戻り値
*            true             : 可視
*            false            : 不可視
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
bool Entity::check_tuple_readable(trid_t trid, Entry& ent) {
    // xmin取得
    trid_t xmin = Transaction::is_tr_valid_to_read(trid, ent.xmin) ? ent.xmin : TRID_MAX;
    // xmax取得
    trid_t xmax = Transaction::is_tr_valid_to_read(trid, ent.xmax) ? ent.xmax : TRID_MAX;
    // xmaxとxminの関係から読込み可能かを判定する
    return xmin <= trid && trid < xmax;
}

/**************************************************************************//**
*
*     関数名：要素書込可否チェック (check_tuple_writable)
* <pre>
*
*    １    機能
*            指定アドレスの要素が書込み可能かどうかを、xmax/lockから判定する
*
*    ２    引数
*            self_trid  :    自トランザクションID          [入力]
*          * entry      :    対象要素のアドレスポインタ    [入力]
*
*    ３    戻り値
*            WRITEABLE      : 書き換え可視(自Trによる上書き)
*            INSERTABLE     : 書き換え可能(Insertが可能)
*            LOCKED         : 更新ロックのため書き換え不可
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Entity::Status Entity::check_tuple_writable(trid_t trid, Entry& ent) {
    if(trid == TRID_MAX) return LOCKED;
    // xmax取得
    trid_t xmax = Transaction::is_tr_valid_to_write(trid,
        ent.xmax, Transaction::IS_XMAX) ? ent.xmax : TRID_MAX;
    // lock取得
    trid_t lock = Transaction::is_tr_valid_to_write(trid,
        ent.lock, Transaction::IS_LOCK) ? ent.lock : TRID_MAX;
    // xmax/lockともに無効なら書込み可能
    if(xmax == TRID_MAX && lock == TRID_MAX)
        return ent.xmin == trid ? WRITEABLE : INSERTABLE;
    // それ以外は書込み禁止
    return LOCKED;
}

/**************************************************************************//**
*
*     関数名：要素エントリ確保 (createTuple)
* <pre>
*
*    １    機能
*            新しい要素エントリ領域を確保する
*            TODO(上位で排他ロックを行う事)
*
*    ２    引数
*            self_trid  :    自トランザクションID          [入力]
*
*    ３    戻り値
*            挿入した要素番号。メモリフル時はEXECUTE_FULLを返す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
rowid_t Entity::createTuple(trid_t trid) {
    rowid_t ret = EXECUTE_FULL;
    if(trid == TRID_MAX) OUT_OF_RANGE("trid");
    // 空き領域の先頭からMaxLineまで検索
    for(rowid_t rowid = free_begin;
            rowid < static_cast<rowid_t>(getMaxLine()); rowid++) {
        // エントリが空きならそのエントリを使う
        Entry& ent = getEntry(rowid);
        if(ent.xmin == TRID_MAX) {
            if(used_end < rowid + 1) used_end = rowid + 1;
            ent.xmin = trid;
            ent.xmax = TRID_MAX;
            ent.lock = TRID_MAX;
            ret = rowid;
            break;
        }
    }
    if(ret == EXECUTE_FULL) MEMORYFULL("メモリフル:" << getName());
    // 空きエントリの先頭を移動して最新の空きエントリを設定
    for(; free_begin < (rowid_t)this->getMaxLine(); free_begin++)
        if(getEntry(free_begin).xmin == TRID_MAX)
            break;

    return ret;
}

/**************************************************************************//**
*
*     関数名：要素エントリ更新 (updateTuple)
* <pre>
*
*    １    機能
*            新しい要素エントリ領域を確保する
*            ・自Trが作成した領域は上書きする
*            ・それ以外はcreateTupleで領域追加する
*
*    ２    引数
*            self_trid  : 自トランザクションID          [入力]
*            rowid      : 要素番号                      [入力]
*
*    ３    戻り値
*            更新後の要素番号。エラー時はマイナスを返す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
rowid_t Entity::updateTuple(trid_t trid, rowid_t rowid) {
    rowid_t newRowID;
    // 引数チェック
    Entry& ent = this->getEntry(rowid);

    // 全体領域の共有ロック取得
    Transaction::getTrans().getLock(Header::READ_LOCK);

    // エンティティ単位で排他ロック
    getLock(Header::WRITE_LOCK);

    // 対象エントリの書込可否判定
    switch(check_tuple_writable(trid, ent)) {
    case WRITEABLE:
        // 自身の作成したエントリは上書き
        newRowID = rowid;
        break;
    case INSERTABLE:
        // 新規エントリ追加
        newRowID = createTuple(trid);
        // エントリ追加NG時はリターンコードをそのまま返す
        if(newRowID >= 0) {
            // 元のエントリから新しいエントリへメモリコピー
            setTuple(newRowID, getTuple(rowid));
            // 古いエントリの無効化
            ent.xmax = trid;
        }
        break;
    case LOCKED:
        // ロック時は上位でタイムアウト判定
        newRowID = EXECUTE_TIMEOUT;
        break;
    }
    // 個別ロック開放
    releaseLock();
    // 全体ロック開放
    Transaction::getTrans().releaseLock();

    return newRowID;
}

/**************************************************************************//**
*
*     関数名：要素エントリ削除 (deleteTuple)
* <pre>
*
*    １    機能
*            作成済みの要素エントリ領域を削除する
*            ・自Trが作成した領域はそのままfreeにする
*            ・それ以外はxmaxを更新して不可視とする
*
*    ２    引数
*            self_trid  : 自トランザクションID          [入力]
*            rowid      : 要素番号                      [入力]
*
*    ３    戻り値
*            rowid      : 更新後の要素番号、エラー時はマイナスを返す
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
int Entity::deleteTuple(trid_t trid, rowid_t rowid) {
    int ret = EXECUTE_TIMEOUT;
    // 引数チェック
    Entry& ent = getEntry(rowid);

    // 全体管理領域で共有ロックを取得する
    Transaction::getTrans().getLock(Header::READ_LOCK);
    // 個別領域の排他ロック取得
    getLock(Header::WRITE_LOCK);

    // 対象エントリの書込可否判定
    switch(check_tuple_writable(trid, ent)) {
    // 自身が作成したエントリは即時開放
    case WRITEABLE:
        freeTuple(rowid);
        ret = EXECUTE_OK;
        break;
    case INSERTABLE:
        // エントリの無効化
        ent.xmax = trid;
        ret = EXECUTE_OK;
        break;
    // ロック時は上位でタイムアウト判定
    case LOCKED:
        ret = EXECUTE_TIMEOUT;
        break;
    }
    // 個別領域ロック解除
    Transaction::getTrans().releaseLock();
    // 全体管理領域ロック開放
    releaseLock();
    return ret;
}

/**************************************************************************//**
*
*     関数名：要素エントリ論理削除 (freeTuple)
* <pre>
*
*    １    機能
*            要素エントリのxminを更新して、空き要素エントリへ登録する
*            TODO(上位で排他ロックを行う事)
*
*    ２    引数
*            rowid      : 要素番号                      [入力]
*
*    ３    戻り値
*            なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Entity::freeTuple(rowid_t rowid) {
    if(rowid < 0 || rowid >= static_cast<rowid_t>(getMaxLine())) {
        OUT_OF_RANGE("RowIDが管理サイズを超えています "
                "(RowID=" <<rowid << " MaxLine="<< getMaxLine() << ")");
    }

    // 空き領域調整
    if(rowid < free_begin) free_begin = rowid;
    // xmin無効化
    getEntry(rowid).xmin = TRID_MAX;
    // 使用中末尾調整
    for(; used_end > 0; used_end--)
        if(getEntry(used_end - 1).xmin != TRID_MAX)
            break;
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}    // namespace SharedMemory
