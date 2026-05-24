#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "ConfigManager.h"
#include <QDialog>
#include <QToolButton>
#include <QMenu>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QNetworkInterface>


class ConfigDialog : public QDialog
{
    Q_OBJECT
  private:
    QToolButton m_ifaceBtn       { this };
    QMenu m_ifaceMenu            { this };
    QComboBox m_modeCb           { this };
    QLineEdit m_dstAddrEdit      { this };
    QSpinBox m_portSb            { this };
    QSpinBox m_mtuSb             { this };
    QLineEdit m_passEdit         { this };
    QDialogButtonBox m_buttonBox { QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
    QNetworkInterface m_selectedIface;

  public:
    ConfigDialog(ConfigManager& confMgr, QWidget *parent = nullptr);
};

#endif // CONFIGDIALOG_H

