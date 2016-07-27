/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#include <stdexcept>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include <rabbitmq_client/simple_client.h>


int main()
{
     using edi::ts::rabbitmq_client::Connection;
     using edi::ts::rabbitmq_client::SimpleClient;

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
          const auto message =
               "My Bonny is over the ocean,\n"
               "My Bonny is over the sea,\n"
               "My Bonny is over the ocean\n"
               "So bring back my Bonny to me..";

          const Connection::Parameters params( hostname, port, username, password, virtualHost );
          Connection connection( params );

          /// Публикуем N сообщений только в exchange (как и полагается)
          for( int i = 0; i < 5; ++i )
          {
               std::cout << "Publishing message!\n";
               SimpleClient::publishMessage( connection, exchange, routingKey, message );
          }

          std::cout << "Done.\n";
     }
     catch( const std::exception& e )
     {
          std::cerr << "exception: " << boost::diagnostic_information( e ) << '\n';
          return 1;
     }

     return 0;
}
