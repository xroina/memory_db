/*--------1---------2---------3---------4---------5---------6---------7---*//**
 * @file
 *     モジュール名：ファイル構成クラス
 * <pre>
 *
 *    １  機能
 *          ファイル構成クラスを定義する
 *
 *    ２  関数名一覧
 *         SQL定義ファイル読込み (readFile)
 *          タグ検索 (searchTag)
 *
 *    ３  更新履歴
 *          REV001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
#include <Init/Exception.h>
#include <Init/FileConfig.h>
#include <cstring>

#include <fstream>
#include <string>
#include <map>

#include <PBase>

namespace SharedMemory
{
using ::std::string;
using ::std::map;

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *
 *     関数名： SQL定義ファイル読込み (readFile)
 * <pre>
 *
 *    １    機能
 *              各種設定ファイルを読み込み、キーと設定内容のマップを返す。
 *
 *    ２    引数
 *            fileName : 読み込み対象のファイル名（フルパス）    [入力]
 *            keyValuePair : 設定内容                            [出力]
 *
 *    ３    戻り値
 *            0     : 正常
 *            0以外 : 異常（ファイルなし、設定内容なし）
 *
 *    ４    履歴
 *            rev001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
FileConfig::fileconfig_t FileConfig::readFile(const string& file) {
    DEBUG_LOG("fileName=" << file);
    fileconfig_t keyVal;
    /* ファイル読み込み */
    ::std::ifstream fin(file);

    // ファイル読み込みエラー
    if(!fin)
        FILEIO_ERROR("ファイルを開くことができませんでした。(" << file << ")");

    char buf[READ_BUFFER_SIZE] = { 0 };
    string id = "";
    string config = "";

    /* EOFまで読み込み */
    while(!fin.eof()) {
        /* １行読み込み */
        fin.getline(buf, sizeof(buf));
        string tmp(buf);
        unsigned int len = tmp.length();

        /* 行頭の場合、「＝」を検索する。 */
        unsigned int cmt = 0;
        unsigned int icl = 0;
        while(cmt < len && buf[cmt] != '#') cmt++;
        while(icl < len && buf[icl] != '=') icl++;
        if(icl > cmt) continue;
        if(icl < len) {
            if(id != "") keyVal[id] = config;
            config = "";
            id = tmp.substr(0, icl);
            icl++;
        } else {
            icl = 0;
        }
        while(icl < len && buf[icl] <= ' ') icl++;
        if(icl < len) config += tmp.substr(icl, len);
    }
    if(id != "") keyVal[id] = config;

    /* ストリームを閉じる。 */
    fin.close();

    /* 設定内容の有無を確認。 */
    if(keyVal.empty())
        FORMAT_ERROR("タグが閉じていないか、設定内容がありません。(" << file << ")");

    return keyVal;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
*
*     関数名： タグ検索  (searchTag)
* <pre>
*
*    １    機能
*              各種設定ファイルを読み込み、キーと設定内容のマップを返す。
*
*    ２    引数
*            buff : 読み込み対象のファイル名（フルパス）       [入力]
*            startIndex : 検索開始位置                         [入力]
*            depth : タグの階層                                [出力]
*
*    ３    戻り値
*            -1     : 該当タグなし。
*            -1以外 : 検索されたタグの位置。
*
*    ４    履歴
*            rev001 : 新規作成
* </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
int FileConfig::searchTag(char* buf, int startIndex, int* depth) {
    int startPos = -1;
    int endPos = -1;
    bool hasFounded = false;
    bool tagStarted = false;

    /* 文字列の頭からループする。 */
    for(int i = startIndex; i < static_cast<int>(::strlen(buf)); i++) {
        if (buf[i] == '<') {
            /* タグ開始文字の場合 */
            tagStarted = true;
            startPos = i;
        } else if(tagStarted) {
            /* タグ候補の場合 */
            if(buf[i] == '>') {
                /* 終端文字の場合 */

                /* フラグクリア */
                tagStarted = false;

                /* 中に文字列が存在するかチェック（SQL文の「ID <> '3'」など）*/
                if(startPos + 1 < i) {
                    /* 開始位置を格納する */
                    endPos = i;
                    hasFounded = true;
                    break;
                }
            } else if(buf[i] != '/' && ::isalnum(buf[i]) == 0) {
                /* 半角英数以外が含まれる場合はタグでないとみなす。 */
                tagStarted = false;
            }
        }
    }

    // 見つかったタグに応じて階層を設定します。
    if(hasFounded) {
        if(buf[startPos + 1] == '/') {
            // 終了タグ
            (*depth)--;
        } else if(buf[endPos - 1] == '/') {
            // 空タグ
            // なにもしない。
        } else {
            // 開始タグ
            (*depth)++;
        }
        return startPos;
    }
    return -1;
}

/*--------1---------2---------3---------4---------5---------6---------7---*//**
 *
 *     関数名： タグ内容取得 (getInnerText)
 * <pre>
 *
 *    １    機能
 *              指定されたタグ（要素名）の内容を取得する。
 *
 *    ２    引数
 *            pConfig : 設定内容                             [入力]
 *            pTagName : タグ名                              [入力]
 *            value : 値                                    [出力]
 *
 *    ３    戻り値
 *            取得文字列 : タグ内容
 *            NULL      : 空タグ指定または取得できなかった場合
 *
 *    ４    履歴
 *            rev001 : 新規作成
 * </pre>
**//*-----1---------2---------3---------4---------5---------6---------7------*/
string FileConfig::getValue(const string& cnf, const string& tag) {
    DEBUG_LOG("config:" << cnf << " tag:" << tag);

    // 開始、終了タグ定義
    string startTag = "<"  + tag + ">";
    string endTag   = "</" + tag + ">";

    // 開始タグ検索
    string::size_type startPos = cnf.find(startTag);
    if (startPos == string::npos)   // タグなし（オプション等）
        return "";

    int textPos = startPos + startTag.length();

    // 終了タグ検索
    string::size_type endPos = cnf.find(endTag, textPos);
    if (endPos == string::npos) // 終了タグなし（）
        FORMAT_ERROR("終了タグが見付かりませんでした。(config=" << cnf << ", tag=" << tag << ")");

    // 開始タグから終了タグの間を返す
    string value = cnf.substr(textPos, endPos - textPos);

    DEBUG_LOG("value:" << value);

    return value;
}

}   // namespace SharedMemory

/*--------1---------2---------3---------4---------5---------6---------7------*/
