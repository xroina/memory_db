/*
 * Exception.cc
 *
 *  Created on: 2018/08/13
 *      Author: xronia
 */

#include <Init/Exception.h>

namespace SharedMemory {

EXECPTION_FUNCTION_(transaction_mismatch,   ::Exception::runtime_error, トランザクション不正);
EXECPTION_FUNCTION_(format_error,           ::Exception::runtime_error, トランザクション不正);

/*--------1---------2---------3---------4---------5---------6---------7------*/

} /* end namespace SharedMemory */
