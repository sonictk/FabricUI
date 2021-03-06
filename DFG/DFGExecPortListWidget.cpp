// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#include "DFGExecPortListWidget.h"
#include "DFGExecPortListItem.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>

using namespace FabricServices;
using namespace FabricUI;
using namespace FabricUI::DFG;

DFGExecPortListWidget::DFGExecPortListWidget(QWidget * parent, DFGController * controller, const DFGConfig & config)
: QWidget(parent)
{
  m_controller = controller;
  m_config = config;

  setMinimumHeight(80);
  setMaximumHeight(80);
  setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);
  setContentsMargins(0, 0, 0, 0);
  layout->setContentsMargins(0, 0, 0, 0);

  m_list = new QListWidget(this);
  layout->addWidget(m_list);
}

DFGExecPortListWidget::~DFGExecPortListWidget()
{
}

void DFGExecPortListWidget::setExec(FabricCore::DFGExec exec)
{
  m_exec = exec;

  m_list->clear();

  try
  {
    for(size_t i=0;i<exec.getExecPortCount();i++)
    {
      char const * portType = " in";
      if(exec.getExecPortType(i) == FabricCore::DFGPortType_Out)
        portType = "out";
      else if(exec.getExecPortType(i) == FabricCore::DFGPortType_IO)
        portType = " io";

      char const * name = exec.getExecPortName(i);
      char const * dataType = exec.getExecPortResolvedType(i);
      DFGExecPortListItem * item = new DFGExecPortListItem(m_list, portType, dataType, name);
      m_list->addItem(item);
    }
  }
  catch(FabricCore::Exception e)
  {
    m_controller->logError(e.getDesc_cstr());
  }
}

QString DFGExecPortListWidget::selectedItem() const
{
  DFGExecPortListItem * item = (DFGExecPortListItem*)m_list->currentItem();
  if(item)
    return item->name();
  return "";
}
