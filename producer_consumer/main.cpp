/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <csignal>
#include <stdexcept>
#include <iostream>
#include <initializer_list>
#include <boost/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>
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


int main()
{
     try
     {
          using edi::ts::rabbitmq_client::Connection;
          using edi::ts::rabbitmq_client::SimpleClient;
          using edi::ts::rabbitmq_client::ConnectionError;

          std::cout << "Start!\n";

          signal( SIGTERM, signalHandler );
          signal( SIGINT, signalHandler );

          const auto hostname = "localhost";
          const auto port = 5672;
          const auto virtualHost = "b2b";
          const auto username = "guest";
          const auto password = "guest";
          const auto exchange = "amq.direct";
          const auto routingKey = "billing";
          const auto queueName = "billing";
          const auto message =
               "My Bonny is over the ocean,\n"
               "My Bonny is over the sea,\n"
               "My Bonny is over the ocean\n"
               "So bring back my Bonny to me...\n";

          const Connection::Parameters queueConnection( hostname, port, virtualHost, username, password );
          const SimpleClient::QueueParameters billingQueue( exchange, routingKey, queueName );
          const SimpleClient::QueueParameters brokenQueue( exchange, "broken", "broken" );

          SimpleClient client( queueConnection );

          const auto messages = {
               "message 000",
               "message 001",
               "message 002",
               "message 003",
               "message 004",
               "message 005",
               "message 006",
               "message 007",
               "message 008",
               "message 009"
          };

          int counter = 0;
          for( auto&& each: messages )
          {
               client.publishMessage( brokenQueue, each );
               if( ++counter == 3 )
               {
                    std::cout << "Press any key to continue...\n";
                    std::cin.get();
               }
          }

          return 0;

          bool reconnectionRequired = false;

          stop = false;
          while( !stop )
          {
               if( reconnectionRequired )
               {
                    client.reconnect();
                    reconnectionRequired = false;
               }

               try
               {
                    client.bind( billingQueue );

                    while( !stop )
                    {
                         if( const auto& env = client.consumeMessage( billingQueue, boost::posix_time::seconds( 5 ) ) )
                         {
                              std::cout << "Got message: " << env->message << "\n";
                              std::cout << "Press any key to continue...\n";
                              std::cin.get();
                              std::cout << "Publishing...\n";
                              client.publishMessage( brokenQueue, env->message );
                              std::cout << "Ack msg...\n";
                              client.ackMessage( env->deliveryTag );
                         }
                         else
                         {
                              std::cout << "Timeout.\n";
                         }
                    }
               }
               catch( const ConnectionError& e )
               {
                    std::cerr << "exception: " << boost::diagnostic_information( e ) << "\n";
                    reconnectionRequired = true;
               }
          }

          std::cout << "Success!\n";
     }
     catch( const std::exception& e )
     {
          std::cerr << "exception: " << boost::diagnostic_information( e ) << '\n';
          return 1;
     }

     return 0;
}
