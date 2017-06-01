//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef __UI_FABRIC_EXCEPTION__
#define __UI_FABRIC_EXCEPTION__

#include <iostream>
#include <QString>
#include <exception>

namespace FabricUI {
namespace Application {

#define NOTHING 0
#define PRINT 1
#define THROW 2

class FabricException : public std::exception
{
  /**
    Exception for commands.
  */

  public: 
    FabricException(
      const QString &message)
      : m_message(message)
    {
    }
 
    virtual ~FabricException() throw() {}

    /// Trows and/or prints a FabricException.
    /// \param method Name of the method that fails.
    /// \param error The error to throw/print.
    /// \param childError A child error
    /// \param flag (THROW, PRINT)
    static QString Throw(
      const QString &method,
      const QString &error = QString(),
      const QString &childError = QString(),
      int flag = THROW)
    {
      QString cmdError = method + ", error: " + error + "\n ";
      if(!childError.isEmpty()) 
        cmdError += childError + "\n ";

      if(flag & PRINT)
        std::cerr << cmdError.toUtf8().constData() << std::endl;

      if(flag & THROW)
        throw FabricException(cmdError);

      return cmdError;
    }

    /// Implementation of exception.
    virtual const char* what() const throw()
    {
      return m_message.toUtf8().constData();
    }

  private:
    QString m_message;
};

} // namespace Commands
} // namespace FabricUI

#endif // __UI_FABRIC_EXCEPTION__
