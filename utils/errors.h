///
/// errors.h
///
/// Created on: Jul 2, 2016
///     Author: alexen
///

#pragma once

#include <amqp.h>
#include <string>

std::string getErrorString( amqp_status_enum status );
