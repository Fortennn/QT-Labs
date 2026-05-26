#ifndef MODEL_DOWNLOAD_DIALOG_H
#define MODEL_DOWNLOAD_DIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
class QComboBox;
class QFile;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressBar;
class QPushButton;
QT_END_NAMESPACE

// Кураторний список GGUF-моделей з HuggingFace + механізм завантаження
// з прогресом, скасуванням та продовженням з місця обриву.
//
// Файли зберігаються в:
//   <applicationDirPath>/models/
// або, як fallback (якщо немає прав запису):
//   <writableLocation(AppDataLocation)>/models/
//
// Після успішного завантаження сигнал `modelDownloaded(path)` сповіщає
// SettingsDialog, який автоматично додає файл у список моделей і робить
// його активним.
class ModelDownloadDialog : public QDialog {
    Q_OBJECT

public:
    struct CatalogEntry {
        QString name;        // людська назва ("Qwen 2.5 0.5B (Q4) — швидка")
        QString fileName;    // куди зберегти
        QString url;         // прямий URL для GET
        qint64  approxSize = 0;  // приблизний розмір у байтах (для UI)
        QString description; // короткий опис (RAM, мови, тощо)
    };

    explicit ModelDownloadDialog(QWidget* parent = nullptr);

    // Куди в принципі будемо качати. Створює теку якщо її нема.
    static QString defaultDownloadDir();

    // Шлях до останньо успішно завантаженого файлу. Порожньо якщо нічого
    // не качали (або скасовано).
    QString downloadedPath() const { return m_downloadedPath; }

signals:
    void modelDownloaded(const QString& absolutePath);

private slots:
    void onCatalogChanged(int idx);
    void onStart();
    void onCancel();
    void onProgress(qint64 received, qint64 total);
    void onFinished();
    void onError();

private:
    static QVector<CatalogEntry> builtinCatalog();
    void buildUi();
    void resetUi();
    void cancelActive();

    QComboBox*     m_catalog       = nullptr;
    QLabel*        m_descLabel     = nullptr;
    QLineEdit*     m_urlEdit       = nullptr;
    QLineEdit*     m_nameEdit      = nullptr;
    QProgressBar*  m_progress      = nullptr;
    QLabel*        m_statusLabel   = nullptr;
    QPushButton*   m_startBtn      = nullptr;
    QPushButton*   m_cancelBtn     = nullptr;
    QPushButton*   m_closeBtn      = nullptr;

    QNetworkAccessManager* m_net   = nullptr;
    QNetworkReply*         m_reply = nullptr;
    QFile*                 m_outFile = nullptr;
    QString                m_currentPath;
    QString                m_downloadedPath;
    qint64                 m_lastReceived = 0;
    qint64                 m_lastTimeMs   = 0;
};

#endif // MODEL_DOWNLOAD_DIALOG_H
