#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <QDialog>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

// Pretty first-run dialog that asks the user for their preferred display
// name and (optionally) avatar. Both are stored per-user via QSettings +
// the application data directory, so they only need to be set once.
class WelcomeDialog : public QDialog {
    Q_OBJECT

public:
    explicit WelcomeDialog(QWidget* parent = nullptr);

    QString chosenName() const;

    // ---- Static "user profile" helpers (used by SettingsDialog,
    //      MainWindow's user chip, and MessageWidget) ----

    // Display name from QSettings. Empty when the user has not been onboarded.
    static QString savedName();
    static void    persistName(const QString& name);

    // Legacy alias kept for the existing call sites that already use it.
    static void    persist(const QString& name) { persistName(name); }

    // Where the avatar PNG is stored on disk (whether or not it currently
    // exists). Lives next to QSettings under QStandardPaths::AppDataLocation.
    static QString avatarFilePath();

    // Round, ready-to-paint pixmap of the saved avatar at <size> px, or a
    // null QPixmap if the user has not set one. Pass 0 to get the raw
    // image at its stored resolution.
    static QPixmap savedAvatar(int size = 0);

    // Round-mask + thin antialiased halo ring baked into the bitmap. Use
    // this everywhere we display the avatar so QLabel doesn't need a
    // square `border` rule (which leaks outside the round mask).
    // <size> is the final pixmap edge in px. Pass an empty pixmap to get
    // a transparent placeholder of that size.
    static QPixmap roundAvatar(const QPixmap& source, int size);

    // Read <sourcePath> as an image, center-square-crop it, scale it to a
    // 256 px PNG, and write it to avatarFilePath(). Returns false on read
    // failure.
    static bool    persistAvatarFromFile(const QString& sourcePath);

    // Delete the stored avatar. Subsequent calls to savedAvatar() return null.
    static void    clearAvatar();

private:
    void pickAvatarInteractive();
    void refreshAvatarPreview();

    QLineEdit*   m_nameEdit       = nullptr;
    QLabel*      m_avatarPreview  = nullptr;   // big circle
};

#endif // WELCOME_DIALOG_H
