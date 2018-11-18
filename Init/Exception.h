/*
 * Exception.h
 *
 *  Created on: 2018/08/13
 *      Author: xronia
 */

#ifndef INIT_EXCEPTION_H_
#define INIT_EXCEPTION_H_

#include <Exception/exception.h>

namespace SharedMemory {

EXECPTION_CLASS_2_(transaction_mismatch,::Exception::runtime_error);
EXECPTION_CLASS_2_(format_error,        ::Exception::runtime_error);

/*--------1---------2---------3---------4---------5---------6---------7------*/

} /* namespace SharedMemory */

#define TRANSACTION_MISMATCH(...)   EXCEPTION_EXCEPTION_H_THROW_(transaction_mismatch,  __VA_ARGS__)
#define FORMAT_ERROR(...)           EXCEPTION_EXCEPTION_H_THROW_(format_error,          __VA_ARGS__)

#endif /* INIT_EXCEPTION_H_ */
