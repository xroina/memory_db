/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能基底クラス
* <pre>
*
*    １  機能
*          共通メモリ管理機能基底クラスを定義する
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/

#ifndef SharedMemory_ACCESS_H_
#define SharedMemory_ACCESS_H_

#include <map>
#include <vector>
#include <string>

#include "inc/SHMConst.h"

namespace SharedMemory
{
class Connection;
/**************************************************************************//**
*
*     クラス名：共通メモリ管理機能基底クラス (CSMAccess)
* <pre>
*
*    １    機能
*          共通メモリ管理機能基底クラスの外部インターフェース
*
*    ２    履歴
*            rev001 : 新規作成
* </pre>
**//**************************************************************************/
class Access {
public:
    static void init(const ::std::string&, const ::std::string&);///< 初期化(システムモニタ用)
    static void init(const ::std::string&);             ///< 初期化(その他プロセス用)
    static void destroy();                     ///< 終了
    static Connection& getConnection();    ///< コネクション取得
    static void executeGarbageCollection();    ///< ガベージコレクション

private:
    /// コネクションオブジェクト
    static Connection connection;
    Access();                               ///< コンストラクタなし
};
/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // end namespace SharedMemory

#endif // SharedMemory_ACCESS_H_
