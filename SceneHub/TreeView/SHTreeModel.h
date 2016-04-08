/*
 *  Copyright 2010-2016 Fabric Software Inc. All rights reserved.
 */

#ifndef __UI_SCENEHUB_SHTREEMODEL_H__
#define __UI_SCENEHUB_SHTREEMODEL_H__

#include <QtCore/QAbstractItemModel>
#include <QtGui/QMenu>
#include <QtGui/QTreeView>
#include <FTL/OwnedPtr.h>
#include <FTL/SharedPtr.h>
#include <FabricCore.h>
#include <vector>
#include <assert.h>
#include <iostream>

#include "SHTreeItem.h"

namespace FabricUI
{
  namespace SceneHub
  {
    class SHTreeModel : public QAbstractItemModel
    {
      Q_OBJECT

      friend class SHTreeItem;

      typedef std::vector<SHTreeItem *> RootItemsVec;

    public:
      SHTreeModel( int , FabricCore::Client client, QObject *parent );

      SHTreeModel( FabricCore::Client client, FabricCore::RTVal sceneGraph, QObject *parent = 0 );

      ~SHTreeModel()
      {
        for ( RootItemsVec::iterator it = m_rootItems.begin();
          it != m_rootItems.end(); ++it )
        {
          SHTreeItem *rootItem = *it;
          delete rootItem;
        }
      }

      QModelIndex addRootItem(
        FabricCore::RTVal rootSGObject
        )
      {
        SHTreeItem *item = new SHTreeItem( this, 0 /* parentItem */, m_client );
        try {
          FabricCore::RTVal args[2];
          args[0] = rootSGObject;//SGObject root
          args[1] = FabricCore::RTVal::ConstructData( m_client, item );//Data externalOwnerID

          FabricCore::RTVal sgTreeViewObjectData = 
            m_treeViewDataRTVal.callMethod( "SGTreeViewObjectData", "getOrCreateRootData", 2, args );

          item->updateNeeded( sgTreeViewObjectData, false );
        }
        catch ( FabricCore::Exception e )
        {
          printf("SHTreeModel::addRootItem: Error: %s\n", e.getDesc_cstr());
        }
        int row = m_rootItems.size();
        QModelIndex index = createIndex( row, 0, item );
        item->setIndex( index );

        m_rootItems.push_back( item );
        return index;
      }

      void setShowProperties( bool show );

      void setShowOperators( bool show );

      void setPropertyColor( QColor color ) { m_propertyColorVariant = QVariant( color ); }
      void setReferenceColor( QColor color ) { m_referenceColorVariant = QVariant( color ); }
      void setOperatorColor( QColor color ) { m_operatorColorVariant = QVariant( color ); }

      std::vector< QModelIndex > getIndicesFromSGObject( FabricCore::RTVal sgObject );

      virtual QVariant data(
        const QModelIndex &index,
        int role
        ) const
      {
        if( !index.isValid() )
          return QVariant();
        SHTreeItem *item = static_cast<SHTreeItem *>( index.internalPointer() );
        if( role == Qt::DisplayRole )
          return item->desc();
        else if( role == Qt::ForegroundRole ) {
          if( item->isGenerator() )
            return m_operatorColorVariant;
          if( item->isReference() )
            return m_referenceColorVariant;
          return m_propertyColorVariant;
        } else if( role == Qt::FontRole && item->isPropagated() ) {
          if( item->isPropagated() && item->isOverride() )
            return m_overridePropagatedFontVariant;
          if( item->isPropagated() )
            return m_propagatedFontVariant;
          if( item->isOverride() )
            return m_overrideFontVariant;
        }
        return QVariant();
      }

      virtual Qt::ItemFlags flags( const QModelIndex &index ) const
      {
        if ( !index.isValid() )
          return 0;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
      }

      virtual QModelIndex index(
        int row,
        int col,
        const QModelIndex &parentIndex = QModelIndex()
        ) const
      {
        if ( !hasIndex( row, col, parentIndex ) )
          return QModelIndex();

        SHTreeItem *item;
        if ( parentIndex.isValid() )
        {
          SHTreeItem *parentItem = static_cast<SHTreeItem *>( parentIndex.internalPointer() );
          item = parentItem->childItem( row );
        }
        else item = m_rootItems[row];

        return createIndex( row, col, item );
      }

      virtual QModelIndex parent( const QModelIndex &childIndex ) const
      {
        if ( !childIndex.isValid() )
          return QModelIndex();

        SHTreeItem *childItem =
          static_cast<SHTreeItem *>( childIndex.internalPointer() );
        SHTreeItem *parentItem = childItem->parentItem();
        if ( !parentItem )
          return QModelIndex();

        int row;
        if ( SHTreeItem *grandParentItem = parentItem->parentItem() )
          row = grandParentItem->childRow( parentItem );
        else
        {
          RootItemsVec::const_iterator it =
            std::find(
              m_rootItems.begin(),
              m_rootItems.end(),
              parentItem
              );
          assert( it != m_rootItems.end() );
          row = it - m_rootItems.begin();
        }
        return createIndex( row, 0, parentItem );
      }

      virtual int rowCount( const QModelIndex &index ) const
      {
        int result;
        if ( index.isValid() )
        {
          SHTreeItem *item = static_cast<SHTreeItem *>( index.internalPointer() );
          result = item->childItemCount();
        }
        else result = m_rootItems.size();
        return result;
      }

      virtual int columnCount( const QModelIndex &index ) const
        { return 1; }

      class SceneHierarchyChangedBlocker
      {
      public:

        SceneHierarchyChangedBlocker(
          SHTreeModel *model
          )
          : m_model( model )
        {
          if ( m_model->m_sceneHierarchyChangedBlockCount++ == 0 )
            m_model->m_sceneHierarchyChangedPending = false;
        }

        ~SceneHierarchyChangedBlocker()
        {
          if( --m_model->m_sceneHierarchyChangedBlockCount == 0
            && m_model->m_sceneHierarchyChangedPending ) {
            emit m_model->sceneHierarchyChanged();
          }
        }

      private:

        SHTreeModel *m_model;
      };

      void emitSceneHierarchyChanged()
      {
        if ( m_sceneHierarchyChangedBlockCount > 0 )
          m_sceneHierarchyChangedPending = true;
        else
          emit sceneHierarchyChanged();
      }

      void emitSceneChanged() {
        emit sceneChanged();
      }

    signals:

      void sceneHierarchyChanged() const;
      void sceneChanged() const;

    public slots:

      void onSceneHierarchyChanged()
      {
        SceneHierarchyChangedBlocker blocker( this );
        for ( RootItemsVec::iterator it = m_rootItems.begin();
          it != m_rootItems.end(); ++it )
        {
          SHTreeItem *rootItem = *it;
          rootItem->updateNeeded();
          rootItem->updateChildItemsIfNeeded();
        }
      }

    private:

      void initStyle();

      RootItemsVec m_rootItems;
      FabricCore::Client m_client;
      FabricCore::RTVal m_treeViewDataRTVal;

      bool m_showProperties;
      bool m_showOperators;

      QVariant m_propertyColorVariant;
      QVariant m_referenceColorVariant;
      QVariant m_operatorColorVariant;
      QVariant m_propagatedFontVariant;
      QVariant m_overrideFontVariant;
      QVariant m_overridePropagatedFontVariant;

      FabricCore::RTVal m_getUpdatedChildDataArgs[7];
      FabricCore::RTVal m_updateArgs[3];

      uint32_t m_sceneHierarchyChangedBlockCount;
      bool m_sceneHierarchyChangedPending;
    };
  }
}

#endif // __UI_SCENEHUB_SHTREEMODEL_H__
