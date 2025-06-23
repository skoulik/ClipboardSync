#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "ConfigManager.h"
#include <QDialog>
#include <QToolButton>
#include <QMenu>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>

class ConfigDialog : public QDialog
{
    Q_OBJECT
  private:
    QToolButton ifaceBtn       { this };
    QMenu ifaceMenu            { this };
    QLineEdit dstAddrEdit      { this };
    QSpinBox portSb            { this };
    QLineEdit passEdit         { this };
    QDialogButtonBox buttonBox { QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
    QString selectedIfaceName;

  public:
    ConfigDialog(ConfigManager& confMgr, QWidget *parent = nullptr);
};

#endif // CONFIGDIALOG_H

