//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef __UI_COMMAND_ARG_HELPERS_PYTHON__
#define __UI_COMMAND_ARG_HELPERS_PYTHON__

#include <QStringList>
#include "BaseScriptableCommand.h"
#include "BaseRTValScriptableCommand.h"

namespace FabricUI {
namespace Commands {

class CommandArgHelpers_Python {

  public:
    CommandArgHelpers_Python();

    static bool _IsRTValScriptableCommand_Python(
      BaseCommand *cmd
      );

    static bool _IsScriptableCommand_Python(
      BaseCommand *cmd
      );

    static bool _IsPathValueCommandArg_Python(
      const QString &key,
      BaseCommand *cmd
      );

    static bool _IsCommandArg_Python(
      const QString &key,
      int flags,
      BaseCommand *cmd
      );

    static bool _HasCommandArg_Python(
      const QString &key,
      BaseCommand *cmd
      );

    static bool _IsCommandArgSet_Python(
      const QString &key,
      BaseCommand *cmd
      );

    static QStringList _GetCommandArgKeys_Python(
      BaseCommand *cmd
      );

    static QString _GetCommandArg_Python(
      const QString &key,
      BaseCommand *cmd
      );

    static FabricCore::RTVal _GetRTValCommandArg_Python(
      const QString &key,
      BaseCommand *cmd
      );

    static FabricCore::RTVal _GetRTValCommandArg_Python(
      const QString &key,
      const QString &type,
      BaseCommand *cmd
      );

    static QString _GetRTValCommandArgType_Python(
      const QString &key,
      BaseCommand *cmd
      );
};

} // namespace Commands
} // namespace FabricUI

#endif // __UI_COMMAND_ARG_HELPERS_PYTHON__
