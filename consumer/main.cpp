/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <csignal>
#include <stdexcept>
#include <iostream>
#include <initializer_list>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/optional/optional.hpp>
#include <boost/thread.hpp>
#include <boost/chrono/duration.hpp>
#include <rabbitmq_client/error.h>
#include <rabbitmq_client/simple_client.h>


bool stop = false;


void signalHandler( int signo )
{
     if( signo == SIGINT || signo == SIGTERM )
     {
          stop = true;
     }
}


void waitTermination( boost::thread_group& threads, std::initializer_list< int > signals = { SIGINT, SIGTERM, SIGQUIT } )
{
     sigset_t sset;

     sigemptyset( &sset );

     for( const auto each: signals )
     {
          sigaddset( &sset, each );
     }

     sigprocmask( SIG_BLOCK, &sset, nullptr );

     std::cout << "threads running: " << threads.size() << "; main thread waiting for termination signals...\n";

     int sig = 0;
     sigwait( &sset, &sig );

     std::cout << "termination signal " << sig << " has been caught\n" << "interrupting threads...\n";

     threads.interrupt_all();
}


void worker(
     const std::string& hostname, const int port,
     const std::string& username, const std::string& password,
     const std::string& virtualHost,
     const std::string& exchange,
     const std::string& routingKey,
     const std::string& queueName,
     const boost::optional< boost::posix_time::time_duration >& timeout = boost::none,
     const boost::optional< boost::chrono::milliseconds >& processingDelay = boost::none )
{
     using edi::ts::rabbitmq_client::Connection;
     using edi::ts::rabbitmq_client::ConnectionError;
     using edi::ts::rabbitmq_client::SimpleClient;

     const Connection::Parameters params(
          hostname,
          port,
          username,
          password,
          virtualHost
     );

     Connection connection( params );
     bool reconnect = false;
     while( true )
     {
          try
          {
               if( reconnect )
               {
                    connection.reconnect();
                    reconnect = false;
               }

               SimpleClient::bind( connection, exchange, queueName );
               while( true )
               {
                    const auto& env = SimpleClient::consumeMessage( connection, timeout );
                    if( env )
                    {
                         std::cout << "Got message:\n" << env->message << "\n";
                         SimpleClient::ackMessage( connection, env->deliveryTag );
                    }
                    else
                    {
                         std::cout << "No message consumed.\n";
                    }
               }
          }
          catch( const ConnectionError& e )
          {
               std::cerr << "connection error: " << e.what() << "\n";
               reconnect = true;
          }
          catch( const std::runtime_error& e )
          {
               std::cerr << "exception: " << boost::diagnostic_information( e ) << "\n";
               return;
          }
     }
}


int main()
{
     try
     {
          const auto hostname = "10.0.10.229";
          const auto port = 5672;
          const auto username = "edi-ts";
          const auto password = "123456";
          const auto virtualHost = "b2b";
          const auto exchange = "amq.direct";
          const auto routingKey = "billing";
          const auto queueName = "billing";
          const boost::posix_time::seconds timeout( 5
               );
          const boost::chrono::milliseconds procDelay( 0 );

          boost::thread_group tg;

          for( int i=0; i < 1; ++i )
          {
               tg.create_thread(
                    [ & ]()
                    {
                         worker( hostname, port, username, password, virtualHost, exchange, routingKey, queueName, timeout, procDelay );
                    }
               );
          }

          waitTermination( tg );

          std::cout << "Done.\n";
     }
     catch( const std::exception& e )
     {
          std::cerr << "exception: " << boost::diagnostic_information( e ) << '\n';
          return 1;
     }

     return 0;
}
