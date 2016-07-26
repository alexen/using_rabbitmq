/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <rabbitmq_client/simple_client.h>

#include <stdexcept>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <rabbitmq_client/error.h>
#include <rabbitmq_client/utils.h>


namespace edi {

template< class T, class... Args >
std::unique_ptr< T > make_unique( Args&&... args )
{
     return std::unique_ptr< T >( new T( std::forward< Args >( args )... ) );
}


namespace ts {
namespace rabbitmq_client {

namespace {
namespace aux {


template< typename NetworkOp, typename ReconnectionOp >
void doOperationReconnectOnError( NetworkOp&& networkOp, ReconnectionOp&& reconnectionOp )
{
     try
     {
          networkOp();
          return;
     }
     catch( const ConnectionError& )
     {}

     reconnectionOp();
     networkOp();
}


} // namespace aux
} // namespace {unnamed}


struct Connection::Impl
{
     static amqp_connection_state_t initConnection()
     {
          return amqp_new_connection();
     }


     static amqp_socket_t* initSocket( const amqp_connection_state_t& conn )
     {
          const auto sock = amqp_tcp_socket_new( conn );
          if( !sock )
          {
               BOOST_THROW_EXCEPTION( std::runtime_error( "cannot create amqp socket" ) );
          }
          return sock;
     }

     Impl()
          : connection( Impl::initConnection() )
          , socket( Impl::initSocket( connection ) )
     {}

     ~Impl()
     {
          if( channelOpenned )
          {
               amqp_channel_close( connection, 1, AMQP_REPLY_SUCCESS );
               channelOpenned = false;
          }
          if( socket )
          {
               amqp_connection_close( connection, AMQP_REPLY_SUCCESS );
          }
          amqp_destroy_connection( connection );
     }

     amqp_connection_state_t connection = nullptr;
     amqp_socket_t* socket = nullptr;
     bool channelOpenned = false;
};


Connection::Connection( const Connection::Parameters& params )
     : params_( params )
     , impl_( std::move( make_unique< Connection::Impl >() ) )
{
     connect();
}


/// Необходим для pimpl: unique_ptr требует наличие деструктора
Connection::~Connection()
{}


void Connection::connect()
{
     int attemptsLeft = 5;
     int delayMs = 300;

     while( true )
     {
          try
          {
               std::cout << "Trying connect...\n";
               connect_();
               break;
          }
          catch( const std::exception& e )
          {
               --attemptsLeft;
               delayMs *= 2;
               std::cout << "Failed (reason: " << e.what() << "). Attempts left: " << attemptsLeft << "\n";
          }

          if( attemptsLeft <= 0 )
          {
               BOOST_THROW_EXCEPTION( ConnectionError( "no reconnections attempts left" ) );
          }

          std::cout << "Waiting next try for " << delayMs << " ms\n";
          boost::this_thread::sleep_for( boost::chrono::milliseconds( delayMs ) );
     }

     std::cout << "Connected!\n";
}


void Connection::connect_()
{
     ensureNoErrors(
          amqp_socket_open( impl_->socket, params_.hostname.c_str(), params_.port ),
          "opening TCP socket"
     );

     ensureNoErrors(
          amqp_login(
               impl_->connection,
               params_.virtualHost.c_str(),
               AMQP_DEFAULT_MAX_CHANNELS,
               AMQP_DEFAULT_FRAME_SIZE,
               AMQP_DEFAULT_HEARTBEAT,
               AMQP_SASL_METHOD_PLAIN,
               params_.username.c_str(),
               params_.password.c_str()
          ),
          "login"
     );
     amqp_channel_open( impl_->connection, 1 );
     ensureNoErrors( amqp_get_rpc_reply( impl_->connection ), "opening channel" );
     impl_->channelOpenned = true;
}


void Connection::reconnect()
{
     impl_.reset();
     impl_ = std::move( make_unique< Connection::Impl >() );
     connect();
}


void SimpleClient::publishMessage( const Connection& connection, const SimpleClient::QueueParameters& params, const std::string& message )
{
     ensureNoErrors(
          amqp_basic_publish(
               connection.impl_->connection,                     /* amqp_connection_state_t                 state       */
               1,                                                /* amqp_channel_t                          channel     */
               amqp_cstring_bytes( params.exchange.c_str() ),    /* amqp_bytes_t                            exchange    */
               amqp_cstring_bytes( params.routingKey.c_str() ),  /* amqp_bytes_t                            routing_key */
               0,                                                /* amqp_boolean_t                          mandatory   */
               0,                                                /* amqp_boolean_t                          immediate   */
               nullptr,                                          /* struct amqp_basic_properties_t_ const * properties  */
               amqp_cstring_bytes( message.c_str() )             /* amqp_bytes_t                            body        */
          ),
          "basic publish"
     );
}


void SimpleClient::publishMessage( const Connection& connection, const std::string& exchange, const std::string& routingKey, const std::string& message )
{
     ensureNoErrors(
          amqp_basic_publish(
               connection.impl_->connection, /* amqp_connection_state_t                 state       */
               1,                            /* amqp_channel_t                          channel     */
               fromString( exchange ),       /* amqp_bytes_t                            exchange    */
               fromString( routingKey ),     /* amqp_bytes_t                            routing_key */
               0,                            /* amqp_boolean_t                          mandatory   */
               0,                            /* amqp_boolean_t                          immediate   */
               nullptr,                      /* struct amqp_basic_properties_t_ const * properties  */
               fromString( message )         /* amqp_bytes_t                            body        */
          ),
          "basic publish"
     );
}


void SimpleClient::bind( const Connection& connection, const std::string& exchange, const std::string& queueName, const std::string& routingKey )
{
     amqp_queue_bind(
          connection.impl_->connection,      /* amqp_connection_state_t state       */
          1,                                 /* amqp_channel_t          channel     */
          fromString( queueName.c_str() ),   /* amqp_bytes_t            queue       */
          fromString( exchange.c_str() ),    /* amqp_bytes_t            exchange    */
          fromString( routingKey.c_str() ),  /* amqp_bytes_t            routing_key */
          amqp_empty_table                   /* amqp_table_t            argument    */
     );
     ensureNoErrors( amqp_get_rpc_reply( connection.impl_->connection ), "bind queue" );
}


boost::optional< SimpleClient::Envelope > SimpleClient::consumeMessage(
     const Connection& connection,
     const std::string& queueName,
     const boost::optional< boost::posix_time::time_duration >& timeout
)
{
     static auto makeTimeval =
          []( const boost::optional< boost::posix_time::time_duration >& duration ) -> std::unique_ptr< timeval >
          {
               if( duration )
               {
                    std::unique_ptr< timeval > tv( new timeval );
                    tv->tv_sec = duration->total_seconds();
                    tv->tv_usec = duration->fractional_seconds();
                    return tv;
               }

               return nullptr;
          };

     amqp_basic_consume(
          connection.impl_->connection, /* amqp_connection_state_t state        */
          1,                            /* amqp_channel_t          channel      */
          fromString( queueName ),      /* amqp_bytes_t            queue        */
          amqp_empty_bytes,             /* amqp_bytes_t            consumer_tag */
          0,                            /* amqp_boolean_t          no_local     */
          0,                            /* amqp_boolean_t          no_ack       */
          0,                            /* amqp_boolean_t          exclusive    */
          amqp_empty_table              /* amqp_table_t            arguments    */
     );
     ensureNoErrors( amqp_get_rpc_reply( connection.impl_->connection ), "basic consume" );

     amqp_maybe_release_buffers( connection.impl_->connection );

     amqp_envelope_t envelope = { 0 };

     const auto timer = makeTimeval( timeout );

     const auto reply =
          amqp_consume_message(
               connection.impl_->connection,
               &envelope,
               timer ? boost::addressof( *timer ) : nullptr,
               0
          );

     if( isTimedOutError( reply ) )
     {
          return boost::none;
     }
     else if( isUnexpectedFrameStateError( reply ) )
     {
          handleUnexpectedFrameStateError( connection );
          return boost::none;
     }
     else
     {
          ensureNoErrors( reply, "consume message" );
     }

     std::unique_ptr< amqp_envelope_t, void(*)( amqp_envelope_t* ) > autocleaner( &envelope, amqp_destroy_envelope );

     return SimpleClient::Envelope( toString( envelope.message.body ), envelope.delivery_tag );
}


void SimpleClient::bind( const Connection& connection, const SimpleClient::QueueParameters& params )
{
     amqp_queue_bind(
          connection.impl_->connection,                     /* amqp_connection_state_t state       */
          1,                                                /* amqp_channel_t          channel     */
          amqp_cstring_bytes( params.queueName.c_str() ),   /* amqp_bytes_t            queue       */
          amqp_cstring_bytes( params.exchange.c_str() ),    /* amqp_bytes_t            exchange    */
          amqp_cstring_bytes( params.routingKey.c_str() ),  /* amqp_bytes_t            routing_key */
          amqp_empty_table                                  /* amqp_table_t            argument    */
     );
     ensureNoErrors( amqp_get_rpc_reply( connection.impl_->connection ), "bind queue" );
}


boost::optional< SimpleClient::Envelope > SimpleClient::consumeMessage(
     const Connection& connection,
     const SimpleClient::QueueParameters& params,
     const boost::optional< boost::posix_time::time_duration >& timeout )
{
     static auto makeTimeval =
          []( const boost::optional< boost::posix_time::time_duration >& duration ) -> std::unique_ptr< timeval >
          {
               if( duration )
               {
                    std::unique_ptr< timeval > tv( new timeval );
                    tv->tv_sec = duration->total_seconds();
                    tv->tv_usec = duration->fractional_seconds();
                    return tv;
               }

               return nullptr;
          };


     amqp_maybe_release_buffers( connection.impl_->connection );

     amqp_envelope_t envelope = { 0 };

     const auto timer = makeTimeval( timeout );

     const auto reply =
          amqp_consume_message(
               connection.impl_->connection,
               &envelope,
               timer ? boost::addressof( *timer ) : nullptr,
               0
          );

     if( isTimedOutError( reply ) )
     {
          return boost::none;
     }
     else if( isUnexpectedFrameStateError( reply ) )
     {
          handleUnexpectedFrameStateError( connection );
          return boost::none;
     }
     else
     {
          ensureNoErrors( reply, "consume message" );
     }

     std::unique_ptr< amqp_envelope_t, void(*)( amqp_envelope_t* ) > autocleaner( &envelope, amqp_destroy_envelope );

     return SimpleClient::Envelope( toString( envelope.message.body ), envelope.delivery_tag );
}


void SimpleClient::ackMessage( const Connection& connection, std::uint64_t deliveryTag )
{
     const auto ret =
          amqp_basic_ack(
               connection.impl_->connection, /* amqp_connection_state_t state        */
               1,                            /* amqp_channel_t          channel      */
               deliveryTag,                  /* uint64_t                delivery_tag */
               0                             /* amqp_boolean_t          multiple     */
          );

     if( ret )
     {
          BOOST_THROW_EXCEPTION(
               std::runtime_error( "broker error while acknowledge message with delivery tag: "
                    + boost::lexical_cast< std::string >( deliveryTag ) ) );
     }
}


bool SimpleClient::isTimedOutError( const amqp_rpc_reply_t& reply )
{
     return reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
          && reply.library_error == AMQP_STATUS_TIMEOUT;
}


bool SimpleClient::isUnexpectedFrameStateError( const amqp_rpc_reply_t& reply )
{
     return reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION
          && reply.library_error == AMQP_STATUS_UNEXPECTED_STATE;
}


void SimpleClient::handleUnexpectedFrameStateError( const Connection& connection )
{
     amqp_frame_t frame;

     if( amqp_simple_wait_frame( connection.impl_->connection, &frame ) != AMQP_STATUS_OK )
     {
          return;
     }

     if( frame.frame_type != AMQP_FRAME_METHOD )
          return;

     switch( frame.payload.method.id )
     {
          /// if we've turned publisher confirms on, and we've published a message
          /// here is a message being confirmed
          ///
          case AMQP_BASIC_ACK_METHOD:
               BOOST_THROW_EXCEPTION( std::runtime_error( "unexpected frame state AMQP_BASIC_ACK_METHOD: not implemented" ) );
               break;

          /// if a published message couldn't be routed and the mandatory flag was set
          /// this is what would be returned. The message then needs to be read.
          ///
          case AMQP_BASIC_RETURN_METHOD:
               {
                    amqp_message_t message;
                    const auto ret = amqp_read_message( connection.impl_->connection, frame.channel, &message, 0 );
                    if( ret.reply_type == AMQP_RESPONSE_NORMAL )
                    {
                         amqp_destroy_message( &message );
                    }

               }
               break;

          /// a channel.close method happens when a channel exception occurs, this
          /// can happen by publishing to an exchange that doesn't exist for example
          ///
          /// In this case you would need to open another channel redeclare any queues
          /// that were declared auto-delete, and restart any consumers that were attached
          /// to the previous channel
          ///
          case AMQP_CHANNEL_CLOSE_METHOD:
               BOOST_THROW_EXCEPTION( ConnectionError( "channel closed" ) );
               break;;

          /// a connection.close method happens when a connection exception occurs,
          /// this can happen by trying to use a channel that isn't open for example.
          ///
          /// In this case the whole connection must be restarted.
          ///
          case AMQP_CONNECTION_CLOSE_METHOD:
               BOOST_THROW_EXCEPTION( ConnectionError( "connection closed" ) );
               break;

          default:
               BOOST_THROW_EXCEPTION(
                    std::runtime_error( "unexpected frame method id "
                         + boost::lexical_cast< std::string >( frame.payload.method.id ) ) );
     }
}


SimpleClient::SimpleClient( const Connection::Parameters& params )
     : connection_( params )
{}


void SimpleClient::publishMessage( const QueueParameters& params, const std::string& message )
{
     aux::doOperationReconnectOnError(
          [ & ](){ SimpleClient::publishMessage( connection_, params, message ); },
          [ this ](){ reconnect(); }
     );
}


void SimpleClient::bind( const QueueParameters& params )
{
     SimpleClient::bind( connection_, params );
}


boost::optional< SimpleClient::Envelope > SimpleClient::consumeMessage(
     const QueueParameters& params,
     const boost::optional< boost::posix_time::time_duration >& timeout
)
{
     return SimpleClient::consumeMessage( connection_, params, timeout );
}


void SimpleClient::ackMessage( std::uint64_t deliveryTag )
{
     aux::doOperationReconnectOnError(
          [ & ](){ SimpleClient::ackMessage( connection_, deliveryTag ); },
          [ this ](){ reconnect(); }
     );
}


void SimpleClient::reconnect()
{
     connection_.reconnect();
}


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
