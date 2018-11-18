/**************************************************************************//**
* @file
*     モジュール名：共通メモリ管理機能 共通ヘッダ定義
* <pre>
*
*    １  機能
*          共通メモリ管理機能 共通ヘッダ定義
*
*    ２  更新履歴
*          REV001 : 新規作成
* </pre>
**//**************************************************************************/

#ifndef _SHMCONST_H_
#define _SHMCONST_H_

#include <cstdint>

#include <vector>

namespace SharedMemory
{
/*--------1---------2---------3---------4---------5---------6---------7------*/

static const int EXECUTE_ONE     =  1; ///< 実行結果(1行)
static const int EXECUTE_OK      =  0; ///< 正常実行
static const int EXECUTE_ERR     = -1; ///< その他エラー
static const int EXECUTE_KEYERR  = -2; ///< キー重複
static const int EXECUTE_FULL    = -3; ///< リソースビジー(メモリ不足)
static const int EXECUTE_NULL    = -4; ///< フェッチの値がNULL
static const int EXECUTE_TIMEOUT = -5; ///< タイムアウト

}  // namespace SharedMemory

#endif  // _SHMCONST_H_
