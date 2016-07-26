/// @file
/// @brief
/// @copyright Copyright (c) InfoTeCS. All Rights Reserved.

#pragma once

#include <cstdint>
#include <memory>
#include <boost/optional/optional.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <amqp.h>


namespace edi {
namespace ts {
namespace rabbitmq_client {


/// Класс описывает подключение к очереди RabbitMQ
class Connection
{
public:
     /// Структура, описывающая параметры подключения к серверу RabbitMQ
     struct Parameters
     {
          Parameters(
               std::string&& host,
               int port_,
               std::string&& user,
               std::string&& pwd,
               std::string&& vhost
          )
               : hostname( std::move( host ) )
               , port( port_ )
               , username( std::move( user ) )
               , password( std::move( pwd ) )
               , virtualHost( std::move( vhost ) )
          {}
          std::string hostname;    ///< имя или IP адрес узла, на котором развернут сервер с очередью сообщений
          int port = 0;            ///< порт подключения к очереди
          std::string username;    ///< имя пользователя
          std::string password;    ///< пароль пользователя
          std::string virtualHost; ///< имя виртуального хоста очереди
     };

     /// Конструктор. Автоматически вызывает метод connect()
     explicit Connection( const Parameters& );

     /// Деструктор. Необходим для реализации идиомы Pimpl
     ~Connection();

     /// @brief Инициирует подключение к очереди в несколько попыток при ошибках подключения.
     /// Метод предпринимает пять попыток подключения к очереди с начальным интервалом ожидания
     /// между попытками равным 300 мс. При каждой следующей попытке подключения интервал удваивается.
     /// @throw ConnectionError в случае если все попытки подключения закончились неудачей
     /// @throw std::runtime_error во всех остальных случаях
     void connect();

     /// Инициирует переподключение к очереди посредством вызова метода connect()
     void reconnect();

private:
     /// Реализует подключение к очереди
     /// @throw ConnectionError в случае ошибок связанных с сетевым соединением
     /// @throw std::runtime_error во всех остальных случаях
     void connect_();

     Parameters params_;

     struct Impl;
     std::unique_ptr< Impl > impl_;

     friend class SimpleClient;
};


/// Класс реализует простой клиент для работы с очередью RabbitMQ
class SimpleClient
{
public:
     /// Структура, описывающая параметры подключения к очереди
     struct QueueParameters
     {
          QueueParameters(
               std::string&& exch,
               std::string&& rkey,
               std::string&& qname
          )
               : exchange( std::move( exch ) )
               , routingKey( std::move( rkey ) )
               , queueName( std::move( qname ) )
          {}
          std::string exchange;    ///< точка подключения к очереди (exchange в терминах AMQP)
          std::string routingKey;  ///< ключ маршрутизации для отправки/получения сообщений
          std::string queueName;   ///< имя очереди
     };

     /// Конверт, используемый для получения сообщений из очереди
     /// @note возвращаемое из очереди сообщение содержит большое кол-во потенциально полезных данных
     /// (таких как exchange, routing_key и т.п.); если в будущем они понадобятся - их следует просто добавлять
     /// в эту структуру, т.о. остальной код останется работоспособным
     struct Envelope
     {
          Envelope( std::string&& m, const std::uint64_t tag )
               : message( std::move( m ) ), deliveryTag( tag )
          {}

          std::string message;          ///< Тело сообщения
          std::uint64_t deliveryTag;    ///< Идентификатор сообщения (для подтверждения доставки)
     };

     /// Конструктор. Создает внутри себя подключение к очереди посредством вызова конструктора Connection()
     explicit SimpleClient( const Connection::Parameters& );

     /// @brief Публикует сообщение @a message в очередь, которая описывается параметрами @a param
     /// @note Осуществляет попытки повторного соединения при перехвате исключения ConnectionError
     /// @throw ConnectionError в случае если все попытки подключения закончились неудачей
     /// @throw std::runtime_error во всех остальных случаях
     void publishMessage( const QueueParameters& params, const std::string& message );

     /// @brief Осуществляет связывание клиента с очередью, которая описывается параметрами @a param.
     /// @note Используется только для прослушивания очереди. В случае циклического прослушивания очереди метод должен вызываться вне цикла.
     /// @throws ConnectionError, std::runtime_error
     void bind( const QueueParameters& param );

     /// @brief Активирует прослушивание очереди. Блокирует вызывающий поток до получения сообщений.
     /// @param params параметры очереди
     /// @param timeout таймаут подключения (boost::none - "вечное" подключение)
     /// @return Envelope в случае получения сообщения, boost::none при таймауте
     /// @throws ConnectionError, std::runtime_error
     boost::optional< Envelope > consumeMessage(
          const QueueParameters& params,
          const boost::optional< boost::posix_time::time_duration >& timeout = boost::none
     );

     void ackMessage( std::uint64_t deliveryTag );

     void reconnect();

     static void publishMessage( const Connection&, const QueueParameters&, const std::string& message );

     static void publishMessage( const Connection&, const std::string& exchange, const std::string& routingKey, const std::string& message );
private:

     static void bind( const Connection&, const QueueParameters& );

     static boost::optional< Envelope > consumeMessage(
          const Connection&,
          const QueueParameters&,
          const boost::optional< boost::posix_time::time_duration >& timeout = boost::none
     );

     static void ackMessage( const Connection&, std::uint64_t deliveryTag );

     /// Возвращает true, если ожидание сообщений прерывается по таймауту
     static bool isTimedOutError( const amqp_rpc_reply_t& );

     /// Возвращает true, если приходит сообщение с неверным состоянием фрейма
     /// @note в этом случае необходимо произвести ряд действий по корректной обработке данного состояния
     static bool isUnexpectedFrameStateError( const amqp_rpc_reply_t& );

     /// Осуществляет обработку ситуации, когда ожидание сообщения прерывается из-за неверного состояния фрейма
     /// @note код взят из примера example/amqp_consumer.c библиотеки rabbitmq-c
     static void handleUnexpectedFrameStateError( const Connection& );

     Connection connection_;
};


} // namespace rabbitmq_client
} // namespace ts
} // namespace edi
