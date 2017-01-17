//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "UIntViewItem.h"
#include "QVariantRTVal.h"
#include "VEIntSpinBox.h"

#include <limits.h>
#include <QHBoxLayout>

using namespace FabricUI::ValueEditor;

UIntViewItem::UIntViewItem(
  QString const &name,
  QVariant const &value,
  ItemMetadata* metadata
  )
  : BaseViewItem( name, metadata )
{
  m_spinner = new VEIntSpinBox;
  m_spinner->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
  m_spinner->setMinimum( 0 );
  m_spinner->setMaximum( INT_MAX );
  m_spinner->setKeyboardTracking( false );
  
  QHBoxLayout *layout = new QHBoxLayout;
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 0 );
  layout->addWidget( m_spinner );
  layout->addStretch();

  m_widget = new QWidget;
  m_widget->setObjectName( "VEUIntViewItem" );
  m_widget->setLayout( layout );

  onModelValueChanged( value );

  connect(
    m_spinner, SIGNAL( interactionBegin() ), 
    this, SIGNAL( interactionBegin() )
    );
  connect(
    m_spinner, SIGNAL( valueChanged( int ) ), 
    this, SLOT( OnSpinnerChanged( int ) )
    );
  connect(
    m_spinner, SIGNAL( interactionEnd( bool ) ), 
    this, SIGNAL( interactionEnd( bool ) )
    );

  metadataChanged();
}

UIntViewItem::~UIntViewItem()
{
}

QWidget *UIntViewItem::getWidget()
{
  return m_widget;
}

void UIntViewItem::onModelValueChanged( QVariant const &v )
{
  m_spinner->setValue( getQVariantRTValValue<unsigned>( v ) );
}

void UIntViewItem::OnSpinnerChanged( int value )
{
  emit viewValueChanged(
    QVariant::fromValue<unsigned>( unsigned( value ) )
    );
}

void UIntViewItem::OnEditFinished()
{
  emit viewValueChanged(
    QVariant::fromValue<unsigned>( unsigned( m_spinner->value() ) )
    );
}

//////////////////////////////////////////////////////////////////////////
// 
BaseViewItem* UIntViewItem::CreateItem(
  QString const &name,
  QVariant const &value,
  ItemMetadata* metadata
  )
{
  if ( RTVariant::isType<unsigned>(value)
    || RTVariant::isType<unsigned long long>(value) )
  {
    return new UIntViewItem( name, value, metadata );
  }
  return 0;
}

const int UIntViewItem::Priority = 3;
