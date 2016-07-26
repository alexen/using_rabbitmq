/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#pragma once

#include <stdexcept>
#include <string>
#include <amqp.h>


namespace edi {
namespace ts {
namespace rabbitmq_client {


/// Тип исключения, генерируемый при ошибках соединения
struct ConnectionError : std::runtime_error
{
     ConnectionError( const std::string& msg ) : std::runtime_error( msg ) {}
};


void ensureNoErrors( int status, const std::string& context );


void ensureNoErrors( const amqp_rpc_reply_t& reply, const std::string& context );


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
