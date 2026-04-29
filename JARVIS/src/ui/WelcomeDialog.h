#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

// Pretty first-run dialog that asks the user for their preferred display
// name. The result is stored via QSettings (key "user/name") so it only
// shows the first time the application is launched.
class WelcomeDialog : public QDialog {
    Q_OBJECT

public:
    explicit WelcomeDialog(QWidget* parent = nullptr);

    QString chosenName() const;

    // Returns the saved display name, or an empty string if the user has not
    // been onboarded yet (i.e. WelcomeDialog should be shown).
    static QString savedName();

    // Persists the name via QSettings.
    static void persist(const QString& name);

private:
    QLineEdit* m_nameEdit = nullptr;
};

#endif // WELCOME_DIALOG_H
