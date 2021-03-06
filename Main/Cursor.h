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
#ifndef _CSMCURSOR_H
#define _CSMCURSOR_H

#include <cstdlib>

#include "PBase"

#include "inc/SHMConst.h"

namespace SharedMemory
{
class Connection;
/**************************************************************************//**
*
*     クラス名：共通メモリ管理機能カーソルクラス (CSMCursor)
* <pre>
*
*    １    機能
*          共通メモリ管理機能カーソルクラスの外部インターフェース
*
*    ２    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
class Cursor {
private:
    ::Entity::rowid_vec_t cursor;       ///< カーソル対象RowID vectoer
    ::Entity::AppTable& data;           ///< エンティティ
    long cursor_index;                  ///< カーソルINDEX
    int  error_code;                    ///< カーソルエラーコード

public:
    explicit Cursor(::Entity::AppTable&);
    virtual ~Cursor();

    bool fetch();                           ///< フェッチ
    void close();                           ///< クローズ

    /**********************************************************************//**
    *
    *     関数名：カーソルサイズ取得(getSize)
    * <pre>
    *
    *    １    機能
    *           取得したカーソルに格納されているデータの数を返却する
    *
    *    ２    引数
    *           なし
    *
    *    ３    戻り値
    *           カーソルの持つデータ数
    *
    *    ４    履歴
    *            REV001 : 新規作成
    * </pre>
    **//**********************************************************************/
    inline size_t getSize() {
        return cursor.size();
    }

    /**********************************************************************//**
    *
    *     関数名：カーソルvector取得(getRowID_vec)
    * <pre>
    *
    *    １    機能
    *           取得したカーソルに格納されているデータのRowIDを
    *           vector配列として返却する
    *
    *    ２    引数
    *           なし
    *
    *    ３    戻り値
    *           カーソルの持つRowIDのベクター配列
    *
    *    ４    履歴
    *            REV001 : 新規作成
    * </pre>
    **//**********************************************************************/
    inline ::Entity::rowid_vec_t& getRowIDs() {
        return cursor;
    }

    /**********************************************************************//**
    *
    *     関数名：カーソルエラーコード設定(setErrorCode)
    * <pre>
    *
    *    １    機能
    *           カーソルにエラーコードを設定する。
    *           共有メモリ管理機能専用
    *
    *    ２    引数
    *           err_code    :   エラーコード
    *
    *    ３    戻り値
    *           なし
    *
    *    ４    履歴
    *            REV001 : 新規作成
    * </pre>
    **//**********************************************************************/
    inline void setErrorCode(int err_code) {
        error_code = err_code;
    }

    /**********************************************************************//**
    *
    *     関数名：カーソルエラーコード取得(getErrorCode)
    * <pre>
    *
    *    １    機能
    *           カーソルのエラーコードを取得する。
    *
    *    ２    引数
    *           なし
    *
    *    ３    戻り値
    *           err_code    :   エラーコード
    *
    *    ４    履歴
    *            REV001 : 新規作成
    * </pre>
    **//**********************************************************************/
    inline int getErrorCode() {
        return error_code;
    }

};
/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // namespace SharedMemory

#endif // _CSMCURSOR_H
