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
               const std::string& host
               , int port_
               , const std::string& user
               , const std::string& pwd
               , const std::string& vhost
          )
               : hostname( host )
               , port( port_ )
               , username( user )
               , password( pwd )
               , virtualHost( vhost )
          {}
          std::string hostname;    ///< имя или IP адрес узла, на котором развернут сервер с очередью сообщений
          int port = 0;            ///< порт подключения к очереди
          std::string username;    ///< имя пользователя
          std::string password;    ///< пароль пользователя
          std::string virtualHost; ///< имя виртуального хоста очереди
     };

     /// Конструкторы. Автоматически вызывают метод connect()
     Connection(
          const std::string& host,
          int port,
          const std::string& user,
          const std::string& pwd,
          const std::string& vhost );

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


/// @brief Класс реализует простой клиент для работы с очередью RabbitMQ
///
/// Класс содержит два по сути одинаковых интерфейса: статический и обычный. Статический интерфейс
/// представляет собой набор статических функций, которые позволяют работать с очередью без создания объекта класса.
/// Статический интерфейс не обеспечивает попыток переподключения при работе с очередью.
/// Ряд методов обычного интерфейса обеспечивают перехват исключения при разрыве соединения и инициализируют
/// попытку переподключения к очереди.
class SimpleClient
{
public:
     /// Структура, описывающая параметры подключения к очереди
     struct QueueParameters
     {
          QueueParameters(
               const std::string& exch
               , const std::string& rkey
               , const std::string& qname
          )
               : exchange( exch )
               , routingKey( rkey )
               , queueName( qname )
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

     /// @brief Публикует сообщение в очередь
     /// @details Метод поддерживает публикацию непосредственно в очередь, отправку сообщения в точку публикации,
     /// а также отправку сообщения в точку публикации с указанием люча маршрутизации. Для того чтобы отправить
     /// сообщение непосредственно в очередь, необходимо имя очереди передать в параметр @a routingKey, при этом
     /// параметр @a exchange необходимо забить пустой строкой. Для передачи сообщения в точку публикации без
     /// указания конкретной очереди, необходимо указать только параметр @a exchange, а параметр @a routingKey
     /// забить пустой строкой. При указании обоих параметров - @a exchange и @a routingKey, сообщение будет
     /// отправлено в точку публикации @a exchange, параметр @a routingKey при этом рассматривается не как имя очереди,
     /// а как ключ маршрутизации (т.е. с поддержкой всевозможных wildcards).
     ///
     /// @note Для публикации сообщения в очередь достаточно указать название точки входа @a exchange
     /// или название очереди, при этом название очереди передается в качестве параметра @a routingKey.
     /// Неиспользуемые параметры забиваются пустыми строками.
     ///
     /// @attention В настоящее время реализация клиента требует, чтобы к моменту вызова метода точка
     /// публикации @a exchange или очередь @a routingKey (при публикации непосредственно в очередь)
     /// должны существовать и быть доступны
     ///
     /// Пример кода
     /// @code
     /// // Инициализация соединения
     ///
     /// Connection connection( hostname, port, username, password, virtualHost );
     ///
     /// // Отправляем сообщение "some message or data" непосредственно в очередь с именем "qtest.queue_name"
     /// // (неиспользуемый параметр exchange забит пустой строкой)
     /// // Очередь "qtest.queue_name" к моменту вызова должна существовать!
     ///
     /// SimpleClient::publishMessage( connection, "", "qtest.queue_name", "some message or data" );
     ///
     /// // Отправляем сообщение в точку публикации "qtest.exchange.fanout", при этом имя очереди не указываем
     /// // Точка публикации "qtest.exchange.fanout" к моменту вызова должна существовать!
     ///
     /// SimpleClient::publishMessage( connection, "qtest.exchange.fanout", "", "some message or data" );
     ///
     /// // Отправляем сообщение с указанием точки публикации и ключа маршрутизации
     /// // Точка публикации "qtest.exchange.fanout" к моменту вызова должна существовать, а вот очередь возможно и необязательно.
     ///
     /// SimpleClient::publishMessage( connection, "qtest.exchange.fanout", "qtest.queue_name", "some message or data" );
     ///
     /// @endcode
     /// @param connection активное подключение к очереди
     /// @param exchange точка для публикации сообщения (exchange в термниах AMQP)
     /// @param routingKey ключ маршрутизации или название очереди
     /// @param message публикуемое сообщение
     /// @throw ConnectionError в случае разрыва или ошибок соединения
     /// @throw std::runtime_error во всех остальных случаях
     static void publishMessage(
          const Connection& connection
          , const std::string& exchange
          , const std::string& routingKey
          , const std::string& message
     );

     /// @brief Связывает точку публикации @a exchange с конкретной очередью @a queueName. Также может быть указан @a routingKey
     /// @note Используется только для прослушивания очереди
     /// @attention К моменту вызова метода и точка публикации @a exchange, и очередь @a queueName должны существовать
     /// @throw ConnectionError в случае разрыва или ошибок соединения
     /// @throw std::runtime_error во всех остальных случаях
     static void bind(
          const Connection&,
          const std::string& exchange,
          const std::string& queueName,
          const std::string& routingKey = "" );

     /// @brief Получает сообщение из очереди @a queueName с блокировкой вызывающего потока до получения сообщения или до истечения времени @a timeout
     ///
     /// Пример кода
     /// @code
     /// // Инициализация соединения
     ///
     /// Connection connection( hostname, port, username, password, virtualHost );
     ///
     /// // Прослушиваем непосредственно очередь, без связывания с точкой публикации (сообщения
     /// // будут приходить только в том случае, если producer публикует сообщения непосредственно в очередь,
     /// // или если связывание очереди с точкой публикации осуществляется администратором очереди)
     ///
     /// const auto envelope = SimpleClient::consumeMessage( connection, "qtest.queue_name" );
     ///
     /// // Прослушиваем очередь, предварительно связывая ее с конкретной точкой публикации
     ///
     /// SimpleClient::bind( connection, "qtest.exchange.fanout", "qtest.queue_name" );
     ///
     /// // Инициируем прослушивание очереди с блокировкой вызывающего потока
     ///
     /// const auto envelope = SimpleClient::consumeMessage( connection, "qtest.queue_name", boost::posix_time::seconds( 30 ) );
     ///
     /// if( envelope )
     /// {
     ///      std::cout << "Got message:\n" << envelope->message << "\n";
     ///
     ///      // Сразу приведу пример подтверждения сообщения
     ///
     ///      SimpleClient::ackMessage( connection, envelope->deliveryTag );
     /// }
     /// else
     /// {
     ///      std::cout << "Timeout.\n";
     /// }
     /// @endcode
     /// @param exchange точка для публикации сообщения (exchange в термниах AMQP)
     /// @param queueName название очереди
     /// @param timout время ожидания сообщения (boost::none - бесконечное ожидание)
     /// @return Envelope с сообщением или boost::none при таймауте
     /// @throw ConnectionError в случае разрыва или ошибок соединения
     /// @throw std::runtime_error во всех остальных случаях
     static boost::optional< Envelope > consumeMessage(
          const Connection& connection,
          const std::string& queueName,
          const boost::optional< boost::posix_time::time_duration >& timeout = boost::none
     );

     /// Подтверждает получение сообщения
     /// @param deliveryTag идентификатор сообщения (извлекается из очереди вместе с сообщением в составе Envelope)
     /// @throw std::runtime_error во всех остальных случаях
     static void ackMessage( const Connection&, std::uint64_t deliveryTag );

     /// Конструкторы. Создают внутри себя подключение к очереди посредством вызова конструктора Connection()
     SimpleClient(
          const std::string& host,
          int port,
          const std::string& user,
          const std::string& pwd,
          const std::string& vhost
     );

     explicit SimpleClient( const Connection::Parameters& );

     /// @see static void publishMessage()
     void publishMessage( const std::string& exchange, const std::string& routingKey, const std::string& message );

     /// @see static void publishMessage()
     void publishMessage( const QueueParameters& params, const std::string& message );

     /// @see static void bind()
     void bind( const std::string& exchange, const std::string& queueName, const std::string& routingKey = "" );

     /// @see static void bind()
     void bind( const QueueParameters& params );

     /// @see static boost::optional< Envelope > consumeMessage()
     boost::optional< Envelope > consumeMessage(
          const std::string& queueName,
          const boost::optional< boost::posix_time::time_duration >& timeout = boost::none
     );

     /// @see static boost::optional< Envelope > consumeMessage()
     boost::optional< Envelope > consumeMessage(
          const QueueParameters& params,
          const boost::optional< boost::posix_time::time_duration >& timeout = boost::none
     );

     /// @see static void ackMessage()
     void ackMessage( std::uint64_t deliveryTag );

     /// Инициирует переподключение к очереди посредством вызова Connection::reconnect()
     /// @see Connection::reconnect()
     void reconnect();

private:
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
