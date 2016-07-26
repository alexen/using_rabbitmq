/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <rabbitmq_client/utils.h>


namespace edi {
namespace ts {
namespace rabbitmq_client {


std::string toString( const amqp_bytes_t& bytes )
{
     return std::string( static_cast< const char* >( bytes.bytes ), bytes.len );
}


amqp_bytes_t fromString( const std::string& str )
{
     return str.empty() ? amqp_empty_bytes : amqp_cstring_bytes( str.c_str() );
}


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
