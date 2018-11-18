/*--------1---------2---------3---------4---------5---------6---------7---*//**
 * @file
 *     モジュール名：ファイル構成クラスヘッダ
 * <pre>
 *
 *    １  機能
 *        ファイル構成クラスを定義する
 *
 *    ２  更新履歴
 *          REV001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
#ifndef SHAREDMEMORY_FILECONFIG_H_
#define SHAREDMEMORY_FILECONFIG_H_

#include <string>
#include <map>

#define READ_BUFFER_SIZE 1024

namespace SharedMemory
{

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *
 *     クラス名：ファイル構成クラス (CFileConfig)
 * <pre>
 *
 *    １    機能
 *          ファイル構成クラスを管理する
 *
 *    ２    履歴
 *            rev001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
class FileConfig {
 public:
    /// 設定ファイル取り込み型
    typedef ::std::map<const ::std::string, ::std::string> fileconfig_t;

    static fileconfig_t readFile(const ::std::string&);
    static ::std::string getValue(const ::std::string&, const ::std::string&);

 private:
    static int searchTag(char*, int, int*);
};

}   // end namespace SharedMemory
/*--------1---------2---------3---------4---------5---------6---------7------*/

#endif /* SHAREDMEMORY_FILECONFIG_H_ */
