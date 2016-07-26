/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#pragma once

#include <string>
#include <rabbitmq_client/fwd.h>

namespace edi {
namespace ts {
namespace rabbitmq_client {


/// Формирует строку из типа amqp_bytes_t
std::string makeString( const amqp_bytes_t& bytes );


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
