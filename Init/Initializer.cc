/*--------1---------2---------3---------4---------5---------6---------7---*//**
 * @file
 *      モジュール名：共通メモリ管理機能 基盤共有メモリ初期化クラス
 * <pre>
 *
 *    １  機能
 *          全体管理情報、個別管理情報の初期化・開放を管理する
 *
 *    ２  更新履歴
 *          REV001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
#include <Init/Exception.h>
#include <Entity/AppTable.h>
#include <string>
#include <sys/mman.h>

#include "PBase"
#include "Entity/EntityCache.h"
#include <Entity/IndexerCache.h>
#include <Entity/IndexName.h>
#include <Entity/NameMaster.h>
#include <Init/FileConfig.h>
#include <Init/Initializer.h>
#include <Main/Access.h>
#include <Main/Connection.h>
#include <Main/Cursor.h>
#include <Manager/Header.h>
#include <Manager/Index.h>
#include <Manager/IndexManager.h>
#include <Manager/Transaction.h>
#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::std::string;
using ::Entity::EntityCache;
using ::Entity::NameMaster;
using ::Entity::AppTable;
using ::Entity::IndexName;
using ::Entity::IndexNameIndexer;

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *  Staticメンバの前方宣言
**//*-----1---------2---------3---------4---------5---------6---------7------*/
Transaction*    Initializer::transaction_addr = nullptr;
                                ///< 全体管理情報アドレス定義
Index*          Initializer::index_index_addr = nullptr;
                                ///< インデックス管理インデックスアドレス定義
IndexManager*   Initializer::index_addr = nullptr;
                                ///< インデックス管理情報アドレス定義
table_map_t     Initializer::table_map;
                                ///< 個別管理情報アドレスマップ定義
index_map_t     Initializer::index_map;
                                ///< インデックス管理情報マップ定義

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *
 *  関数名：数パラメータ取得 (getDecmal)
 * <pre>
 *
 *    １    機能
 *            文字列から<tag>タグでくくられた範囲の文字を数として取得する
 *
 *    ２    引数
 *           key         : tag文字             [入力]
 *           value       : <tag>形式文字列      [入力]
 *
 *    ３    戻り値
 *            フィールド数
 *
 *    ４    履歴
 *            REV001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
unsigned long Initializer::getDecimal(const string& key, const string& value) {
    long ret = 0;
    string line = FileConfig::getValue(value, key);

    try {
        ret = ::std::stol(line.c_str());
    } catch(::std::invalid_argument& e) {
        INVALID_ARGUMENT("タグのに数値以外が指定されています。" << key << "=" << line);
    } catch(::std::out_of_range& e) {
        OUT_OF_RANGE("タグのに数値が大きすぎます。" << key << "=" << line);
    }

    if(ret < 1)
        OUT_OF_RANGE("タグにゼロ以下の数値が指定されています。" << key << "=" << line);

    return static_cast<unsigned long>(ret);
}
/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：エンティティ名パラメータ取得 (getTableName)
* <pre>
*
*    １    機能
*            文字列から<tag>タグでくくられた範囲の文字をエンティティまたは
*            インデックスの名称として取得する
*
*    ２    引数
*           key         : tag文字             [入力]
*           value       : <tag>形式文字列      [入力]
*           name        : エンティティまたはインデックスまたはインデクサ
*                         の名称               [出力]
*
*    ３    戻り値
*            フィールド数
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
string Initializer::getTableName(const string& key, const string& value) {

    // タグの取得
    string name = FileConfig::getValue(value, key);

    if(name.length() < ::Entity::ENTITY_NAME_LENGTH_MIN ||
            name.length() >= ::Entity::ENTITY_NAME_LENGTH)
        // 文字サイズが大きすぎるor小さすぎる場合はNG
        LENGTH_ERROR(name << "の文字数は5〜63文字に制限されています。"
                "size=%" << name.length() << " file=" << name);

    TRACE_LOG("key=" << key << " / value=" << name);
    return name;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：共有メモリ確保 (createMemory)
* <pre>
*
*    １    機能
*            指定したファイル名を共有メモリ設定ファイルとしてロードし
*            設定に伴い、全体管理情報、個別管理情報として基板共有メモリを
*            確保する
*
*    ２    引数
*            filename : 共有メモリ設定ファイル名     [入力]
*            dataPath : 共有メモリデータパス名       [入力]
*
*    ３    戻り値
*            EXECUTE_ERR    : メモリ確保失敗
*            EXECUTE_OK     : 成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
void Initializer::createMemory(const string& fileName, const string& dataPath) {
    TRACE_LOG("file=" << fileName << " path=" << dataPath);
    // 設定ファイル取得
    FileConfig::fileconfig_t setFileMap = FileConfig::readFile(fileName);


    /* 全てのタグをループ -------3---------4---------5---------6-------- ココカラ */
    for(auto i = setFileMap.begin(); i != setFileMap.end(); i++) {
        string value;
        const string tagset = i->second;

        string entName;                  // エンティティ名
        string idxName;                  // インデックス名
        string idxid;                    // インデックスID
        string idxr;                     // インデクサ名
        size_t line = 0;                 // 〃 (変換後)
        msec_t timeOut = DEFAULT_TIMEOUT;// タイムアウト(ms)
        string memName;                  // 共有メモリ名
        size_t memSize = 0;              // メモリサイズ
        size_t tblSize = 0;              // 構造体サイズ
        Type tblType = NONE;
        Header* adr = nullptr;
        /* 親タグを識別する -----3---------4---------5---------6------- ココカラ */
        while(1) {
            // 全体トランザクション管理情報設定の場合(TrMgr)
            value = FileConfig::getValue(tagset, "TrMgr");
            if(value.length() > 0) {
                // トランザクション管理情報最大値を取得(MaxLine)
                line = getDecimal("MaxLine", value);
                memSize = Transaction::getSize(line);
                memName = Transaction::TRANSACTION_NAME;
                tblType = TRMNG;
                break;
            }
            // エンティティ名称マスタ設定の場合(EntityName)
            value = FileConfig::getValue(tagset, "EntityMaster");
            if(value.length() > 0) {
                // 登録可能エンティティ数取得(MaxLine)
                line = getDecimal("MaxLine", value);
                tblSize = sizeof(::Entity::NameMaster);
                memSize = Entity::getSize(line, tblSize);
                memName = NameMaster::ENTITY_NAME;
                tblType = ENTITY;
                break;
            }
            // インデックス管理情報設定の場合(IndexMgr)
            value = FileConfig::getValue(tagset, "IndexMgr");
            if(value.length() > 0) {
                // 登録可能インデックス数取得(MaxLine)
                line = getDecimal("MaxLine", value);
                memSize = IndexManager::getSize(line);
                memName = IndexName::ENTITY_NAME;
                tblType = ENTITY;
                break;
            }
            // インデックス管理インデックス設定の場合(IndexMgrIndex)
            value = FileConfig::getValue(tagset, "IndexMgrIndex");
            if(value.length() > 0) {
                // インデックス管理インデックス数(MaxLine)
                line = getDecimal("MaxLine", value);
                memSize = Index::getSize(line);
                memName = IndexNameIndexer::INDEXER_NAME;
                tblType = INDEX;
                break;
            }
            // インデックス情報設定の場合(Index)
            value = FileConfig::getValue(tagset, "Index");
            if(value.length() > 0) {
                // 管理インデックス数を取得(MaxLine)
                line = getDecimal("MaxLine", value);
                // インデックス名を取得(IndexName)
                idxName =  getTableName("IndexName", value);
                memSize = Index::getSize(line);
                memName = idxName;
                tblType = INDEX;
                break;
            }
            // エンティティ情報設定(Entity)
            value = FileConfig::getValue(tagset, "Entity");
            if(value.length() > 0) {
                // 管理エンティティ数を取得(MaxLine)
                line = getDecimal("MaxLine", value);
                // エンティティ名を取得(EntityName)
                entName = getTableName("EntityName", value);
                // テーブル定義(.so)からテーブルサイズを取得
                tblSize = EntityCache::getTableDef(entName).size;
                memSize = Entity::getSize(line, tblSize);
                memName = entName;
                tblType = ENTITY;
                break;
            }
            // インデックスマッピング情報の設定
            value = FileConfig::getValue(tagset, "IndexEntry");
            if(value.length() > 0) {
                entName = getTableName("EntityName", value);
                idxName = getTableName("IndexName",  value);
                idxid   = getTableName("IndexID",    value);
                idxr    = getTableName("Indexer",    value);

                // マッピング情報を構成
                IndexName tpl(entName, idxid, idxName, idxr, ::Entity::INVALID_ROWID);
                AppTable appTable(IndexName::ENTITY_NAME, tpl);
                // 自分のIFを使用して登録する
                Connection& cnn = Access::getConnection();
                cnn.executeInsert(appTable);
                cnn.commitTransaction();
                cnn.close();
                // TODO(ローカルメモリのインデックスマップの登録)
                addIndex(tpl);
                tblType = MAP;
                INFO_LOG("インデックス名マッピング Entity:" << tpl.entity_name.str()
                        << " / Index:" << tpl.index_name.str()
                        << " / IndexID:" << tpl.index_id.str()
                        << " / Indexer:" << tpl.indexer_name.str());
                break;
            }
            FORMAT_ERROR("親タグがありません");
        }
        /* ココマデ -----3---------4---------5---------6------- 親タグを識別する */
        string path = dataPath + "/" + Header::FILE_HEADER + memName + Header::FILE_EXP;
        // インデックス管理情報以外の場合は基板共有メモリを確保する
        if(tblType != MAP) {
            // ロックタイムアウトの取得
            timeOut = getTimeOut(value);
            // 共有メモリを確保する
            adr = createMemory(path, memSize);
            // msync(adr, memory_size, MS_ASYNC);
        }

        /* 管理領域毎に初期化処理を切り替える 4---------5---------6-------- ココカラ */
        if (tblType == TRMNG) {
            static_cast<Transaction*>(adr)->init(memName, timeOut, line);
            addTable(tblType, memName, adr);
        } else if(tblType == INDEX) {
            static_cast<Index*>(adr)->init(memName, line);
        } else if(tblType == ENTITY) {
            if(memName == IndexName::ENTITY_NAME) {
                static_cast<IndexManager*>(adr)->init(memName, line);
            } else {
                static_cast<Entity*>(adr)->init(memName, line, tblSize);
            }
        }
        /* ココマデ -2---------3---------4------- 管理領域毎に初期化処理を切り替える */

        // 共有メモリ名をエンティティ名称マスタに登録する
        if(tblType == INDEX || tblType == ENTITY) {
            // エンティティ名称マスタは共有メモリに登録しない
            if(memName != NameMaster::ENTITY_NAME) {
                NameMaster data(memName);
                AppTable appTable(NameMaster::ENTITY_NAME, data);

                // 自分のIFを使用して登録する
                Connection& cnn = Access::getConnection();
                cnn.executeInsert(appTable);
                cnn.commitTransaction();
                cnn.close();
            }
            // 共有メモリ名とアドレスをローカルメモリのマップへ登録する
            addTable(tblType, memName, adr);
        }
    }
    /* ココマデ -----2---------3---------4---------5---------- 全てのタグをループ */

    return;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：共有メモリ割り当て (attachMemory)
* <pre>
*
*    １    機能
*            createMemoryで確保した領域をプロセスの仮想メモリに割り当てる
*
*    ２    引数
*            dataPath : 共有メモリデータパス名       [入力]
*
*    ３    戻り値
*            EXECUTE_ERR    : メモリ確保失敗
*            EXECUTE_OK     : 成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
void Initializer::attachMemory(const string& dataPath) {

    if(transaction_addr != nullptr)
        MULTI_DEFINE("全体管理領域はマッピング済みです");

    // 全体管理領域を共有メモリへアタッチ
    string trMgrPath = dataPath + "/" + Header::FILE_HEADER +
            Transaction::TRANSACTION_NAME + Header::FILE_EXP;
    addTable(TRMNG, Transaction::TRANSACTION_NAME, createMemory(trMgrPath, 0));

    // エンティティ名称マスタを共有メモリへアタッチ
    string masterPath = dataPath + "/" + Header::FILE_HEADER +
            NameMaster::ENTITY_NAME + Header::FILE_EXP;
    addTable(ENTITY, NameMaster::ENTITY_NAME, createMemory(masterPath, 0));

//    CApplicationData data;

    // コネクションを開く
    Connection& cnn = Access::getConnection();

    NameMaster data;
    AppTable appData(NameMaster::ENTITY_NAME, data);

    // エンティティ名称マスタを開く
    Cursor& cur1 = cnn.openCursor(appData, false);
    // カーソルオープン時エラーコードを持っていたら異常終了
    if(cur1.getErrorCode() >= 0) {
       // 全件フェッチ
        while(cur1.fetch()) {
            // 全体管理情報なら処理なし
            if(data.name == Transaction::TRANSACTION_NAME) continue;
            // エンティティ名称マスタなら処理なし
            if(data.name == NameMaster::ENTITY_NAME) continue;
            // 個別管理情報をアタッチ
            string tmpPath = dataPath + "/" + Header::FILE_HEADER +
                    data.name.str() + Header::FILE_EXP;
            addTable(ENTITY, data.name.str(), createMemory(tmpPath, 0));
        }
    }
    cur1.close();

    IndexName tpl;
    AppTable appNode(IndexName::ENTITY_NAME, tpl);

    // インデックス管理情報を開く
    Cursor& cur2 = cnn.openCursor(appNode, false);
    if(cur2.getErrorCode() >= 0) {
        // 全件フェッチ
        while(cur2.fetch()) addIndex(tpl);
    }
    cur2.close();

    cnn.commitTransaction();
    cnn.close();

    return;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：共有メモリ開放 (detachMemory)
* <pre>
*
*    １    機能
*            基板共有メモリをプロセスの仮想メモリから開放する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            EXECUTE_ERR    : メモリ開放失敗
*            EXECUTE_OK     : 成功
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
void Initializer::detachMemory() {

    // 全体管理領域以外を開放する
    for(auto i = table_map.begin(); i != table_map.end(); i++) {
        Entity* memadr = i->second;
        if(nullptr == memadr) continue;
        int fd = memadr->getFileDiscpriter();
        size_t size = memadr->getMemorySize();
        if(::munmap(memadr, size) < 0) RUNTIME_ERROR(i->first << " 共有メモリ解放失敗");
        INFO_LOG(i->first << " 共有メモリ開放実施");
        ::close(fd);
        i->second = nullptr;
    }
    // ローカルエンティティ管理マスタを開放する
    table_map.clear();

    // 全体管理領域を開放する
    if(transaction_addr != nullptr) {
        int fd = transaction_addr->getFileDiscpriter();
        size_t size = transaction_addr->getMemorySize();
        if(::munmap(transaction_addr, size) < 0) RUNTIME_ERROR(Transaction::TRANSACTION_NAME << " 共有メモリ解放失敗");
        INFO_LOG(Transaction::TRANSACTION_NAME << " 共有メモリ開放実施");
        ::close(fd);
        transaction_addr = nullptr;
    }

    for(auto i = index_map.begin(); i != index_map.end(); i++)
        i->second.clear();

    index_map.clear();

    index_index_addr = nullptr;
    index_addr = nullptr;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：エンティティ追加 (addTable)
* <pre>
*
*    １    機能
*            新しいエンティティを追加する。
*
*    ２    引数
*            table_name  :    エンティティ名     [入力]
*          * table       :    メモリアドレス     [入力]
*
*    ３    戻り値
*            OK         :   正常終了
*            NG         :   異常終了
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
void Initializer::addTable(const Type tblType, const string& tblName, Header* tbl) {

    if(tbl == nullptr) NULLPOINTER("テーブルアドレスが指定されていません");

    if(tblType == TRMNG) {
        transaction_addr = static_cast<Transaction*>(tbl);
        tbl->attatchLog();
    } else if(tblType != MAP) {
        // ローカルテーブルマップへ登録する
        table_map.emplace(tblName, static_cast<Entity*>(tbl));
        // インデックス管理インデックス情報ならstaticにも登録する
        if(tblName == IndexNameIndexer::INDEXER_NAME) index_index_addr = static_cast<Index*>(tbl);
        // インデックス管理情報ならstaticにも登録する
        if(tblName == IndexName::ENTITY_NAME) index_addr = static_cast<IndexManager*>(tbl);
        tbl->attatchLog();
    }
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：ローカルインデックスマップ追加 (addIndex)
* <pre>
*
*    １    機能
*            ローカルのインデックスマップ情報を追加する。
*
*    ２    引数
*          * entity_name    :   エンティティ名     [入力]
*          * index_id       :   インデックスID     [入力]
*          * index_name     :   インデックス名     [入力]
*          * indexer_name   :   インデクサ名      [入力]
*
*    ３    戻り値
*            EXECUTE_OK     :   正常終了
*            EXECUTE_ERR    :   異常終了
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
void Initializer::addIndex(const IndexName& tpl) {
    const string& name = tpl.entity_name.str();
    const string& index = tpl.index_id.str();

    auto i = index_map.find(name);
    if(i == index_map.end()) {
        id_map_t idMap;
        index_map.emplace(name, idMap);
    }
    auto& id = index_map[name];
    if(id.find(index) != id.end())
        MULTI_DEFINE("エンティティに対し同じIndexIDが指定されています。"
                "(Entity:" << name << " IndexID:" << index << ")");

    IndexIndexerName ix = { tpl.index_name.str(), tpl.indexer_name.str() };
    id.emplace(index, ix);
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *
 *   関数名 : タイムアウト値の取得 (getTimeOut)
 *            文字列から<LockFile><TimeOut>タグでくくられた範囲のパラメータ
 *            を取得し、返却する。最初に設定された値を内部保存し、指定され
 *            なかった場合はその値を次のエンティティに引き継ぐ
 *
 *   引数   : value       : <tag>形式文字列      [入力]
 *            *time_out   : タイムアウト値(ms)   [出力]
 *
 *   戻り値 : 0以上       : 取得成功
 *            マイナス値  : 取得失敗
**//*-----1---------2---------3---------4---------5---------6---------7------*/
msec_t Initializer::getTimeOut(const string& value) {
    static const char key[] = "TimeOut";
    static msec_t timeOut = DEFAULT_TIMEOUT;

    string line = FileConfig::getValue(value, key);
    if(line.length() > 0) {
        timeOut = getDecimal(key, value);
    }

    return timeOut;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：プロセス開始時間採取 (getProcTime)
* <pre>
*
*    １    機能
*            指定したPIDのプロセス開始時刻を得る
*            TODO(LINUX依存)
*
*    ２    引数
*            pid    :    プロセスPID     [入力]
*
*    ３    戻り値
*            プロセスの開始時間
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
time_t Initializer::getProcTime(pid_t pid) {
    char buf[PATH_MAX];
    ::snprintf(buf, PATH_MAX, "/proc/%d", pid);
    struct stat stat;
    if(::stat(buf, &stat) != 0) return -1;
    return stat.st_ctime;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名：共有メモリ取得 (createSharedMemory)
* <pre>
*
*    １    機能
*            指定したサイズの共有メモリを取得する
*
*    ２    引数
*            file_name      :    ファイル名       [入力]
*            memory_size    :    メモリサイズ     [入力]
*
*    ３    戻り値
*            ファイルディスクプリタ
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
Header* Initializer::createMemory(const string& fileName, long memSize) {

#ifdef BSD
    long psize = ::getpagesize();
#else
    long psize = ::sysconf(_SC_PAGE_SIZE);
#endif
    long size = (memSize / psize + 1) * psize;
    // 共有メモリを確保する
    int fd = ::open(fileName.c_str(), O_RDWR|O_CREAT, 0666);
    if(fd < 0) RUNTIME_ERROR("共有メモリ取得失敗(open name=" << fileName << " / size=" << memSize << ")");

    struct stat buf;        // ファイルステータス
    // ファイルステータス取得
    if(::fstat(fd, &buf) != 0) buf.st_size = -1;
    long fsize = buf.st_size;
    if(fsize < 0) {
        ::close(fd);
        RUNTIME_ERROR("共有メモリ取得失敗(stat name=" << fileName << " / size=" << memSize << ")");
    }
    if(fsize < size) {
        /* ファイルの必要サイズ分先にシークし、0を書き込み */
        /* ファイルのサイズをマップしたいサイズにする為 */
        if(::lseek(fd, size, SEEK_SET) < 0) {
            ::close(fd);
            RUNTIME_ERROR("共有メモリ取得失敗(seek name=" << fileName << " / size=" << memSize << ")");
        }
        char c = 0;
        if(::read( fd, &c, sizeof(c)) < 0) c = 0;
        if(::write(fd, &c, sizeof(c)) < 0) {
            ::close(fd);
            RUNTIME_ERROR("共有メモリ取得失敗(write name=" << fileName << " / size=" << memSize << ")");
        }
    }
    if(memSize == 0) memSize = fsize;

    Header* adr = reinterpret_cast<Header*>(::mmap(0, memSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
    if(adr == MAP_FAILED || adr == nullptr) {
        ::close(fd);
        RUNTIME_ERROR("共有メモリ取得失敗(mmap name=" << fileName << " / size=" << memSize << ")");
    }
    // ファイルディスクプリタを設定する
    adr->setFileDiscpriter(fd);

    TRACE_LOG("共有メモリ取得成功(mmap name=" << fileName << " / size=" << memSize << ")");

    return adr;
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}  // namespace SharedMemory
