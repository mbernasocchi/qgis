/***************************************************************************
    qgsglobevectorlayerpropertiespage.h
     --------------------------------------
    Date                 : 9.7.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGLOBEVECTORLAYERPROPERTIESPAGE_H
#define QGSGLOBEVECTORLAYERPROPERTIESPAGE_H

#include "ui_qgsglobevectorlayerpropertiespage.h"

#include <QStandardItemModel>

class QgsMapLayer;

/**
 * Item model with overridden match function, so QVariant actually compares the values
 * and not the address of the values when looking for a custom types.
 */
template <typename T>
class QgsEnumComboBoxModel : public QStandardItemModel
{
  public:
    QgsEnumComboBoxModel( QObject* parent )
        : QStandardItemModel( parent )
    {}

    QModelIndexList match( const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags ) const
    {
      Q_UNUSED( flags )

      QModelIndexList result;

      QModelIndex p = parent( start );
      int from = start.row();
      int to = rowCount( p );

      // value to search for
      T val = value.value<T>();

      for ( int r = from; ( r < to ) && ( result.count() < hits ); ++r )
      {
        QModelIndex idx = index( r, start.column(), p );
        if ( !idx.isValid() )
          continue;

        // Convert list index to value
        QVariant v = data( idx, role );
        T val2 = v.value<T>();

        if ( val == val2 )
          result.append( idx );
      }

      return result;
    }
};


class QgsGlobeVectorLayerPropertiesPage : public QgsVectorLayerPropertiesPage, private Ui::QgsGlobeVectorLayerPropertiesPage
{
    Q_OBJECT

  public:
    explicit QgsGlobeVectorLayerPropertiesPage( QgsVectorLayer* layer, QWidget *parent = 0 );

  public slots:
    virtual void apply();

  private slots:
    void on_mCbxAltitudeClamping_currentIndexChanged( int index );
    void on_mCbxAltitudeTechnique_currentIndexChanged( int index );

  signals:
    void layerSettingsChanged( QgsMapLayer* );

  private:
    QgsVectorLayer* mLayer;
};

#endif // QGSGLOBEVECTORLAYERPROPERTIESPAGE_H
