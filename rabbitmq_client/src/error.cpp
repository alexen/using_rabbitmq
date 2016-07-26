/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <rabbitmq_client/error.h>

#include <sstream>
#include <boost/throw_exception.hpp>
#include <rabbitmq_client/utils.h>


namespace edi {
namespace ts {
namespace rabbitmq_client {


namespace {
namespace throw_exception {


void runtimeError( const std::string& message )
{
     BOOST_THROW_EXCEPTION( std::runtime_error( message ) );
}


void runtimeError( const std::string& message, int status )
{
     runtimeError( message + ": " + amqp_error_string2( status ) );
}


void connectionError( const std::string& message )
{
     BOOST_THROW_EXCEPTION( ConnectionError( message ) );
}


void connectionError( int status )
{
     connectionError( amqp_error_string2( status ) );
}

} // namespace throw_exception

namespace aux {


void ensureNoErrors( const amqp_method_number_t methodId, const void* decoded )
{
     std::ostringstream ostr;

     switch( methodId )
     {
          case AMQP_CONNECTION_CLOSE_METHOD:
          {
               ostr << "server connection error";
               if( const auto details = static_cast< const amqp_connection_close_t* >( decoded ) )
               {
                    ostr << ": " << details->reply_code
                         << ", message: " << makeString( details->reply_text );
               }
               throw_exception::connectionError( ostr.str() );
               break;
          }
          case AMQP_CHANNEL_CLOSE_METHOD:
          {
               ostr << "server channel error";
               if( const auto details = static_cast< const amqp_channel_close_t * >( decoded ) )
               {
                    ostr << ": " << details->reply_code
                         << ", message: " << makeString( details->reply_text );
               }
               throw_exception::connectionError( ostr.str() );
               break;
          }
          default:
               ostr << "unknown server error; method id " << methodId << " (hex: 0x" << std::hex << methodId << ")";
               throw_exception::runtimeError( ostr.str() );
     }
}


} // namespace aux
} // namespace {unnamed}


void ensureNoErrors( int status, const std::string& context )
{
     switch( status )
     {
          case AMQP_STATUS_OK:
               return;
          case AMQP_STATUS_SOCKET_ERROR:
          case AMQP_STATUS_CONNECTION_CLOSED:
               throw_exception::connectionError( status );
          break;
          default:
               throw_exception::runtimeError( "amqp status error while " + context, status );
     }
}


void ensureNoErrors( const amqp_rpc_reply_t& reply, const std::string& context )
{
     switch( reply.reply_type )
     {
          case AMQP_RESPONSE_NORMAL:
               return;
          case AMQP_RESPONSE_NONE:
               throw_exception::runtimeError( "missing RPC reply type" );
          break;
          case AMQP_RESPONSE_LIBRARY_EXCEPTION:
               ensureNoErrors( reply.library_error, context );
          break;
          case AMQP_RESPONSE_SERVER_EXCEPTION:
               aux::ensureNoErrors( reply.reply.id, reply.reply.decoded );
          break;
     }
}


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
