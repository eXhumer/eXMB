/*
 * eXMB - Reddit media poster via external media hosting platforms
 * Copyright (C) 2022 - eXhumer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FLAIRLISTMODEL_HXX
#define FLAIRLISTMODEL_HXX

#include <QAbstractListModel>
#include <QJsonArray>

class FlairListModel : public QAbstractListModel {
private:
  QJsonArray m_flairs;

public:
  FlairListModel(const QJsonArray &flairs, QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
};

#endif // FLAIRLISTMODEL_HXX
