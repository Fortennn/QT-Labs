#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "passwordentry.h"
#include "passwordleakchecker.h"

class AddEntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEntryDialog(QWidget *parent = nullptr);

    void setEditingEntry(const PasswordEntry &entry);
    PasswordEntry getEntry() const;

private:
    void setupUI();

private slots:
    void onCheckLeakClicked();
    void onLeakCheckCompleted(bool isLeaked, int count);
    void onLeakCheckError(const QString &errorMessage);

private:
    QLineEdit   *m_titleEdit;
    QLineEdit   *m_usernameEdit;
    QLineEdit   *m_passwordEdit;
    QLineEdit   *m_websiteEdit;
    QComboBox   *m_categoryCombo;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
    QPushButton *m_togglePasswordButton;

    QPushButton *m_checkLeakButton;
    QLabel      *m_leakStatusLabel;

    PasswordLeakChecker *m_leakChecker;

    int m_entryId = 0;
};
