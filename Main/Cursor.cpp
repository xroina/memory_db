/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能カーソルクラス
* <pre>
*
*    １  機能
*          共通メモリ管理機能カーソルクラスを定義する
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/

#include <Main/Connection.h>
#include <Main/Cursor.h>
#include <Manager/Entity.h>
#include <string.h>

#include <algorithm>

#include "Entity/ImplMatcher.h"
#include "inc/SHMmacro.h"

namespace SharedMemory
{
/**************************************************************************//**
*
*     関数名：コンストラクタ
* <pre>
*
*    １    機能
*            インスタンスの持つ変数をすべて初期化する
*
*    ２    引数
*            const char* table_name
*            ::CApplicationData* app_data
*
*    ３    戻り値
*            なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Cursor::Cursor(::Entity::AppTable& data) : data(data), cursor_index(-1),
        error_code(EXECUTE_OK) {
    cursor.clear();
}

/**************************************************************************//**
*
*     関数名：デストラクタ
* <pre>
*
*    １    機能
*            カーソルがクローズしてなければクローズする
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
Cursor::~Cursor() {
    this->close();
}

/**************************************************************************//**
*
*     関数名：フェッチ (fetch)
* <pre>
*
*    １    機能
*            カーソル初期化時に作成されたカーソル配列に沿ってフェッチを実施する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            CURSOR_ERR(-1) : 失敗
*            CURSOR_END(0)  : カーソル末尾
*            CURSOR_OK(1)   : フェッチ成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
bool Cursor::fetch() {
    TRACE_LOG("Entity:" << data.getTableName());

    Entity& table = Entity::getAddr(data.getTableName());

    // カーソルを進める
    cursor_index++;
    if(cursor_index < 0) ::std::out_of_range("Cursor Index");
    // カーソルがvectorサイズを上回ったらフェッチ完了
    if(static_cast<size_t>(cursor_index) >= cursor.size()) return false;
    // データがNULLの場合は内部利用(カーソルindex+1を返却)
    if(data.getTableSize() == 0) return true;
    // カーソル対象のデータ本体を取得
    const ::Entity::AbstEntity& adr = table.getTuple(cursor[cursor_index]);

    TRACE_LOG("cursor index:%ld adr:%p size:%lu", cursor_index, &adr, data.getTableSize());

    // データ本体をCApplicationDataにコピー
    data.setData(adr);

    SHM_DEBUG_DMP(FETCH, table.getName().c_str(), cursor[cursor_index], &adr, data.getTableSize());

    // fetchできたことを返却(1)
    return true;
}

/**************************************************************************//**
*
*     関数名：カーソルクローズ(close)
* <pre>
*
*    １    機能
*            カーソルオブジェクトを開放する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_OK : 成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Cursor::close() {
    cursor.clear();
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // namespace SharedMemory
