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
          const auto hostname = "localhost";
          const auto port = 5672;
          const auto username = "guest";
          const auto password = "guest";
          const auto virtualHost = "vhost.test";
          const auto exchange = "exchange.test.fanout";
          const auto routingKey = "";
          const auto queueName = "queue.test.001";
          const auto message =
               "My Bonny is over the ocean,\n"
               "My Bonny is over the sea,\n"
               "My Bonny is over the ocean\n"
               "So bring back my Bonny to me..";

          const Connection::Parameters params( hostname, port, username, password, virtualHost );
          Connection connection( params );

          /// Публикуем сообщение только в exchange (как и полагается)
          SimpleClient::publishMessage( connection, "", queueName, message );

          std::cout << "Done.\n";
     }
     catch( const std::exception& e )
     {
          std::cerr << "exception: " << boost::diagnostic_information( e ) << '\n';
          return 1;
     }

     return 0;
}
