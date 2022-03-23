#include "FlairListModel.hxx"
#include <QJsonObject>

FlairListModel::FlairListModel(const QJsonArray &flairs, QObject *parent)
    : QAbstractListModel(parent), m_flairs(flairs) {}

int FlairListModel::rowCount(const QModelIndex & /*parent*/) const {
  return m_flairs.count();
}

QVariant FlairListModel::data(const QModelIndex &index, int role) const {
  if (index.isValid() && index.row() < m_flairs.count()) {
    auto flair = m_flairs[index.row()].toObject();
    auto flairId = flair["id"];
    auto flairText = flair["text"];

    if (role == Qt::UserRole) // flair text
      return flairText.toVariant();

    else if (role == Qt::UserRole + 1) // flair id
      return flairId.toVariant();

    else if (role == Qt::ItemDataRole::DisplayRole)
      return QVariant(flairId.toString() + " (" + flairText.toString() + ")");
  }

  return QVariant();
}
