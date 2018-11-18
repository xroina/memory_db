/*--------1---------2---------3---------4---------5---------6---------7---*//**
 * @file
 *     モジュール名：共通メモリ管理機能 基盤共有メモリ初期化モジュール
 * <pre>
 *
 *    １  機能
 *          全体管理情報、個別管理情報の初期化・開放を管理する
 *
 *    ２  更新履歴
 *          REV001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
#ifndef _CSHAREDMEMORYINITIALIZER_H_
#define _CSHAREDMEMORYINITIALIZER_H_

#include <Entity/IndexName.h>
#include <ctime>

#include <map>
#include <set>
#include <string>


namespace SharedMemory
{
class Transaction;
class Entity;
class Index;
class IndexManager;
class Header;

/// エンティティ-アドレス型
typedef ::std::map<::std::string, Entity*> table_map_t;

/// インデクサ-インデックス構造体
class IndexIndexerName {
public:
    const ::std::string index_name;
    const ::std::string indexer_name;

    IndexIndexerName(const ::std::string& index_name,
            const ::std::string& indexer_name) :
        index_name(index_name), indexer_name(indexer_name) { }
};

/// インデックスIDマップ
typedef ::std::map<::std::string, IndexIndexerName> id_map_t;

/// エンティティ-インデックスID管理マップ
typedef ::std::map<::std::string, id_map_t> index_map_t;

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *   クラス名：共通メモリ管理機能 基盤共有メモリ初期化クラス
 *   (CSharedMemoryInitializer)
 *             全体管理情報、個別管理情報の初期化・開放を管理する
**//*-----1---------2---------3---------4---------5---------6---------7------*/
class Initializer {
public:
    /// テーブル種別
    enum Type {NONE, TRMNG, ENTITY, INDEX, MAP};

private:
    /// 設定ファイル取り込み型

    /// ディフォルトタイムアウト=10s
    static const unsigned long DEFAULT_TIMEOUT = 10000;

    Initializer();     ///< コントラクタ(なし)

    /// 数値データ取得
    static unsigned long getDecimal(const ::std::string&, const ::std::string&);
    /// エンティティ/インデックス名称取得
    static ::std::string getTableName(const ::std::string&, const ::std::string&);
    /// エンティティ/インデックス 登録処理
    static void addTable(const Type, const ::std::string&, Header*);
    /// ローカルインデックスマップ 登録処理
    static void addIndex(const ::Entity::IndexName&);
    /// タイムアウト値の取得
    static uint64_t getTimeOut(const ::std::string&);

public:
    /// 共有メモリ初期化
    static void createMemory(const ::std::string&, const ::std::string&);
    /// 共有メモリアタッチ
    static void attachMemory(const ::std::string&);
    /// 共有メモリ開放
    static void detachMemory();
    /// プロセス起動時刻取得
    static time_t getProcTime(pid_t);

    /// エンティティ名-アドレスマップ
    static table_map_t table_map;
    /// エンティティ名-インデックスマップ
    static index_map_t index_map;
    /// 全体管理情報アドレス
    static Transaction* transaction_addr;
    /// インデックス管理インデックスアドレス
    static Index* index_index_addr;
    /// インデックス管理情報アドレス
    static IndexManager* index_addr;
private:
    static Header* createMemory(const std::string&, long);
};

/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // end namespace SharedMemory

#endif // _CSHAREDMEMORYINITIALIZER_H_
