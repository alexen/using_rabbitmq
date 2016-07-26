/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#pragma once

#include <string>
#include <amqp.h>

namespace edi {
namespace ts {
namespace rabbitmq_client {


/// Формирует строку из типа amqp_bytes_t
std::string toString( const amqp_bytes_t& bytes );

/// Формирует тип amqp_bytes_t из строки
/// @note В случае пустой строки возвращается объект amqp_empty_bytes
amqp_bytes_t fromString( const std::string& str );


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
