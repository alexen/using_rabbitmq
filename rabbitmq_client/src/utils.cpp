/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <rabbitmq_client/utils.h>


namespace edi {
namespace ts {
namespace rabbitmq_client {


std::string makeString( const amqp_bytes_t& bytes )
{
     return std::string( static_cast< const char* >( bytes.bytes ), bytes.len );
}


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
