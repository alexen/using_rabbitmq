///
/// errors.cpp
///
/// Created on: Jul 2, 2016
///     Author: alexen
///

#include "errors.h"


std::string getErrorString( amqp_status_enum status )
{
     switch( status )
     {
          case AMQP_STATUS_OK:                              return "Operation successful";
          case AMQP_STATUS_NO_MEMORY:                       return "Memory allocation failed";
          case AMQP_STATUS_BAD_AMQP_DATA:                   return "Incorrect or corrupt data was received from the broker";
          case AMQP_STATUS_UNKNOWN_CLASS:                   return "An unknown AMQP class was received";
          case AMQP_STATUS_UNKNOWN_METHOD:                  return "An unknown AMQP method was received";
          case AMQP_STATUS_HOSTNAME_RESOLUTION_FAILED:      return "Unable to resolve the hostname";
          case AMQP_STATUS_INCOMPATIBLE_AMQP_VERSION:       return "The broker advertised an incompaible AMQP version";
          case AMQP_STATUS_CONNECTION_CLOSED:               return "The connection to the broker has been closed";
          case AMQP_STATUS_BAD_URL:                         return "malformed AMQP URL";
          case AMQP_STATUS_SOCKET_ERROR:                    return "A socket error occurred";
          case AMQP_STATUS_INVALID_PARAMETER:               return "An invalid parameter was passed into the function";
          case AMQP_STATUS_TABLE_TOO_BIG:                   return "The amqp_table_t object cannot be serialized because the output buffer is too small";
          case AMQP_STATUS_WRONG_METHOD:                    return "The wrong method was received";
          case AMQP_STATUS_TIMEOUT:                         return "Operation timed out";
          case AMQP_STATUS_TIMER_FAILURE:                   return "The underlying system timer facility failed";
          case AMQP_STATUS_HEARTBEAT_TIMEOUT:               return "Timed out waiting for heartbeat";
          case AMQP_STATUS_UNEXPECTED_STATE:                return "Unexpected protocol state";
          case AMQP_STATUS_SOCKET_CLOSED:                   return "Underlying socket is closed";
          case AMQP_STATUS_SOCKET_INUSE:                    return "Underlying socket is already open";
          case AMQP_STATUS_BROKER_UNSUPPORTED_SASL_METHOD:  return "Broker does not support the requested SASL mechanism";
          case AMQP_STATUS_UNSUPPORTED:                     return "Parameter is unsupported in this version";
          case AMQP_STATUS_TCP_ERROR:                       return "A generic TCP error occurred";
          case AMQP_STATUS_TCP_SOCKETLIB_INIT_ERROR:        return "An error occurred trying to initialize the socket library";
          case AMQP_STATUS_SSL_ERROR:                       return "A generic SSL error occurred";
          case AMQP_STATUS_SSL_HOSTNAME_VERIFY_FAILED:      return "SSL validation of hostname against peer certificate failed";
          case AMQP_STATUS_SSL_PEER_VERIFY_FAILED:          return "SSL validation of peer certificate failed";
          case AMQP_STATUS_SSL_CONNECTION_FAILED:           return "SSL handshake failed";
     }

     return "unknown error";
}
