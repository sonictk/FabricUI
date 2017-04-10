#
# Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
#

from PySide import QtGui
from FabricEngine import Core
from FabricEngine.Canvas.Utils import *
from FabricEngine.FabricUI import Commands
from FabricEngine.Canvas.Commands.CommandRegistry import *

class ToolTableWidget(QtGui.QTableWidget):

    """ ToolTableWidget  
    """

    def __init__(self, parent, canvasWindow):
        """ Initializes the ToolTableWidget.
            
            Arguments:
            - parent: A reference to wiget.
            - canvasWindow: A reference the canvasWindow.
        """
        super(ToolTableWidget, self).__init__(parent)

        self.canvasWindow = canvasWindow

        client = self.canvasWindow.client
        self.toolManager = client.RT.types.WidgetsManager.create()
        self.toolManager = self.toolManager.getWidgetsManager('WidgetsManager')
        self.toolManagerVersion = self.toolManager.getVersion('UInt32').getSimpleType()

        # Two columns: [nane, class, type]
        self.setColumnCount(3)
        self.setHorizontalHeaderItem(0, QtGui.QTableWidgetItem('Name'))
        self.setHorizontalHeaderItem(1, QtGui.QTableWidgetItem('Visible'))
        self.setHorizontalHeaderItem(2, QtGui.QTableWidgetItem('Edit'))

        self.setDragEnabled(False)
        self.setTabKeyNavigation(False)
        self.setSortingEnabled(True)
        self.verticalHeader().setVisible(False)
        self.horizontalHeader().setStretchLastSection(True)
        self.setSelectionMode(QtGui.QAbstractItemView.NoSelection);

        # qss
        self.setObjectName('ToolTableWidget')

    def showEvent(self, event):
        self.onRefresh()
        super(ToolTableWidget, self).showEvent(event)

    def __getToolCount(self):
        toolList = self.toolManager.drawnWidgets
        return toolList.count_slow('Size').getSimpleType()

    def __getToolAtIndex(self, index):
        toolList = self.toolManager.drawnWidgets
        return toolList.get('BaseWidget', index)

    def __resetTableContent(self):
        self.clearContents()
        self.setRowCount(0)

    def onRefresh(self):
        version = self.toolManager.getVersion('UInt32').getSimpleType()

        if version != self.toolManagerVersion:
        
            self.__resetTableContent()
             
            for i in range(0, self.__getToolCount()):
                tool = self.__getToolAtIndex(i)
                toolName = tool.getName('String').getSimpleType()
                isVisible = tool.isVisible('Boolean').getSimpleType()

                self.__createNewRow(toolName)
 
        self.toolManagerVersion = version

    def __setCellWidget(self, row, column, widget):
        pWidget = QtGui.QWidget()
        pLayout = QtGui.QHBoxLayout(pWidget)
        pLayout.addWidget(widget)
        pLayout.setContentsMargins(0, 0, 0, 0)
        pWidget.setLayout(pLayout)
        self.setCellWidget(row, column, pWidget)

    def __createNewRow(self, toolName):
        """ \internal.
            Create a new row: [name, class, type] items.
        """

        rowCount = self.rowCount() 
        self.insertRow(rowCount)
        
        # Name item
        item = QtGui.QTableWidgetItem(toolName) 
        #item.setToolTip(cmd.getHelp())
        item.setFlags(QtCore.Qt.NoItemFlags)
        self.setItem(rowCount, 0, item)
        
        self.__setCellWidget(rowCount, 1, QtGui.QCheckBox(''))
        self.__setCellWidget(rowCount, 2, QtGui.QPushButton('-'))
  
    def filterItems(self, query):
        """ \internal.
            Filters the items according the commands' names, classes or types.
            To filter by class, use '#' before the query.  
            To filter by type, use '@' before the query.  
        """
        searchByClass = False
        if len(query):
            searchByClass = query[0] == '#'

        searchByType = False
        if len(query):
            searchByType = query[0] == '@'
   
        regex = CreateSearchRegex(query) 

        for i in range(0, self.rowCount()):

            # Checks the action's name/shortcut matches.
            cmdName = self.item(i, 0).text()
            cmdType = self.item(i, 1).text()
            implType = self.item(i, 2).text()

            if  (   (searchByClass and not searchByType and regex.search(cmdType.lower()) ) or 
                    (not searchByClass and searchByType and regex.search(implType.lower()) ) or 
                    (not searchByClass and not searchByType and  regex.search(cmdName.lower()) ) ):

                isCommand = GetCommandRegistry().isCommandRegistered(cmdName)
                self.setRowHidden(i, False)
                 
            else:
                self.setRowHidden(i, True)