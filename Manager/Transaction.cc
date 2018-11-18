/**************************************************************************//**
* @file
*     クラス名：共通メモリ管理機能 全体管理領域（全体トランザクション管理領域）
*     (CTransactionManager)
* <pre>
*
*    １    機能
*          全体管理領域（トランザクション管理領域の構造を定義する
*
*    ２    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
#include <Init/Initializer.h>
#include <Manager/Index.h>
#include <Manager/IndexManager.h>
#include <Manager/Transaction.h>
#include "inc/SHMmacro.h"

namespace SharedMemory
{
using ::std::string;

const string Transaction::TRANSACTION_NAME  = "$";

/**********************************************************************//**
*
*     関数名：全体トランザクション管理情報サイズ取得 (getSize)
* <pre>
*
*    １    機能
*            管理情報数から、全体管理情報を確保するうえで必要なメモリ
*            サイズを取得する
*
*    ２    引数
*            num       :    管理情報数     [入力]
*
*    ３    戻り値
*            メモリサイズ(byte)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**********************************************************************/
size_t Transaction::getSize(size_t num) {
    return sizeof(Transaction) + sizeof(Recode) * num;
}

/**************************************************************************//**
*
*     関数名：全体トランザクション管理情報アドレス取得 (getAddr)
* <pre>
*
*    １    機能
*            全体管理情報の先頭アドレスを取得する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            先頭アドレスのポインタ
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Transaction& Transaction::getTrans() {
    if(Initializer::transaction_addr == nullptr)
        NULLPOINTER("トランザクション情報が初期化されていません");

    return *Initializer::transaction_addr;
}

/**************************************************************************//**
*
*     関数名：全体トランザクション管理情報初期化 (init)
* <pre>
*
*    １    機能
*            全体管理情報を初期化する
*
*    ２    引数
*            name      :    管理情報名称         [入力]
*            file      :    ロックファイル名称   [入力]
*            num       :    管理情報数           [入力]
*
*    ３    戻り値
*            メモリサイズ(byte)
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Transaction::init(const string& name, const msec_t timeOut, const size_t num) {
    Header::init(name, timeOut, num, getSize(num), sizeof(Recode));
    trid_next = trid_collecting = TRID_MIN;
    // インデックスルートの初期化
    index_root_master = ::Entity::INVALID_ROWID;
    // トランザクションコミットカウントの初期化
    trcc_next = TRCC_MIN;
}

/**************************************************************************//**
*
*     関数名：トランザクション情報を取得
* <pre>
*
*    １    機能
*            リングバッファからトランザクション情報を取得する
*
*    ２    引数
*            trid      :    トランザクションID   [入力]
*
*    ３    戻り値
*            トランザクション情報のポインタ
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
Transaction::Recode& Transaction::getTransaction(trid_t trid) {
    if(this->trid_collecting > trid || trid >= this->trid_next)
        OUT_OF_RANGE("トランザクション情報が有効範囲外です trid:" << trid <<
                " collecting:" << this->trid_collecting <<
                " next:" << this->trid_next);

    return this->tag_transaction[trid % this->getMaxLine()];
}

/**************************************************************************//**
*
*     関数名：トランザクション開始
* <pre>
*
*    １    機能
*            次回取得トランザクションIDをインクリメントして、
*            トランザクション情報を取得する
*
*    ２    引数
*            なし
*
*    ３    戻り値
*            トランザクションID
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
trid_t Transaction::startTr() {
    trid_t trid = this->trid_next++;
    Recode& tr = getTransaction(trid);
    // 自プロセスのPID保存
    tr.pid = ::getpid();
    // 自プロセスの開始時間保存
    tr.pid_time = Initializer::getProcTime(tr.pid);
    // トランザクションを処理中に設定
    tr.status = IN_PROGRESS;

    TRACE_LOG("LOAD Index Root Master (trid:" << trid << ")"
            " MASTERroot:" << this->index_root_master <<
            " -> TRNroot:" << tr.index_root);

    // TODO(インデックスルートはここで設定する)
    tr.index_root = this->index_root_master;
    // TRCC現在値の保存
    tr.trcc_begin = this->trcc_next;

    return trid;
}

/**************************************************************************//**
*
*     関数名：トランザクションコミット
* <pre>
*
*    １    機能
*            トランザクション情報にCOMMITEDを設定して
*            トランザクションを確定する
*
*    ２    引数
*             trid : トランザクションID
*
*    ３    戻り値
*             なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Transaction::commitTr(trid_t trid) {
    Recode& tr = this->getTransaction(trid);
    // トランザクション完了時のtrid_nextを保存
    tr.trid_end = this->trid_next;
    // トランザクションをコミット状態にする
    tr.status = COMMITTED;
    // TODO(次に生成されるTrから可視なindex_rootであれば全体に反映する)
    if(IndexManager::is_index_root_valid(trid, tr.index_root)) {
        TRACE_LOG("CommitTr(trid:" << trid <<") TRNroot:" << tr.index_root <<
                " -> MASTERroot:" << this->index_root_master);
        this->index_root_master = tr.index_root;
    }
    // トランザクションコミットカウントをすすめる
    tr.trcc_end = trcc_next++;
}

/**************************************************************************//**
*
*     関数名：トランザクションロールバック
* <pre>
*
*    １    機能
*            トランザクション情報にABORTEDを設定して
*            トランザクションを確定する
*
*    ２    引数
*             trid : トランザクションID
*
*    ３    戻り値
*             なし
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
void Transaction::abortTr(trid_t trid) {
    Recode& tr = getTransaction(trid);
    // トランザクション完了時のtrid_nextを保存
    tr.trid_end = trid_next;
    // トランザクションをアボート状態にする
    tr.status = ABORTED;
}

/**************************************************************************//**
*
*     関数名：トランザクション読込判定 (is_tr_valid_to_read)
* <pre>
*
*    １    機能
*            指定したトランザクションIDが読込み可能かを判定する
*
*    ２    引数
*            self_trid    : 現在のトランザクションID
*            target_trid  : 対象のトランザクションID
*
*    ３    戻り値
*            true  : 取得可能
*            false : 取得不能
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
bool Transaction::is_tr_valid_to_read(trid_t trid, trid_t tgtTrid) {
    // 自身のTRIDが対象ならば取得可能
    if(tgtTrid == trid) return true;

    Transaction& trn = Transaction::getTrans();

    // 回収済みのTRIDは全てコミット扱いなので取得可能
    if(tgtTrid < trn.trid_collecting) return true;
    // 未来のTRID(TRID_MAXを含む)は存在しないので取得不能
    if(trn.trid_next <= tgtTrid) return false;

    // 対象TrIDに対応するトランザクション管理情報取得
    Recode& tr = trn.getTransaction(tgtTrid);
    // 対象TrIDが可視範囲外ならtrcc_end = TRCC_MIN status = COMMITTEDと扱う

    trcc_t trcc_begin = TRCC_MAX;
    if (trn.trid_next > trid) {
        // 自TrIDに対応するトランザクション管理情報取得
        Recode& self_tr = trn.getTransaction(trid);
        trcc_begin = self_tr.trcc_begin;
    }
    // 対象Trがコミットで対象Trのtrcc_endよりも自trcc_beginの方が後なら可視
    return tr.status == COMMITTED && tr.trcc_end < trcc_begin;
}

/**************************************************************************//**
*
*     関数名：xmaxとlockのトランザクション判定 (is_tr_valid_to_write)
* <pre>
*
*    １    機能
*            xmaxとlockの指すTrIDが有効か無効かを判定する。
*
*    ２    引数
*            trid    : 現在のトランザクションID
*            tarTrid : 対象のトランザクションID
*            sts     : target_tridがxmaxかlockかを示すフラグ
*
*    ３    戻り値
*            true  : 取得可能
*            false : 取得不能
*
*    ４    履歴
*            REV001 : 新規作成
* </pre>
**//**************************************************************************/
bool Transaction::is_tr_valid_to_write(trid_t trid, trid_t tgtTrid, target_trid_t sts) {
    Transaction& trn = Transaction::getTrans();
    if(trid == TRID_MAX || tgtTrid == TRID_MAX) return false;
    // 自身のTRIDが書いたTrIDなら無効
    if(tgtTrid == trid) return false;
    // 回収済みのTRIDで、調査対象がxmaxなら有効、lockは無効
    if(tgtTrid < trn.trid_collecting) return sts == IS_XMAX;
    // 未来のTrID(TRID_MAXを含む)なら無効
    if(trn.trid_next <= tgtTrid) return false;
    // 対象トランザクション情報取得
    Recode& tr = trn.getTransaction(tgtTrid);
    // TODO(処理中のxmax/lockの指すTrIDは有効)
    if(tr.status == IN_PROGRESS) return true;
    // 対象がxmaxでCOMMITなら有効
    return (tr.status == COMMITTED && sts == IS_XMAX);
    // 対象がlockなら無効。xmaxでもコミット以外なら無効
}

/*--------1---------2---------3---------4---------5---------6---------7------*/
}   // end namespace SharedMemory
