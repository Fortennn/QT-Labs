#ifndef CHAT_HISTORY_DIALOG_H
#define CHAT_HISTORY_DIALOG_H

#include <QDateTime>
#include <QDialog>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QLabel;
QT_END_NAMESPACE

// One on-disk chat entry. Bound to a JSON file under
// QStandardPaths::AppDataLocation/chats/<id>.json.
struct ChatEntry {
    QString   id;          // file basename (no extension)
    QString   title;       // user-visible title
    QDateTime updatedAt;   // last modification time
    int       messageCount = 0;
};

// Modal dialog showing the list of saved chats. The user can:
//   - Click a row to load it (sets m_selectedId, accept()).
//   - Click "Видалити" to remove the file and refresh the list.
//   - Click "Новий чат" to request a fresh empty conversation.
class ChatHistoryDialog : public QDialog {
    Q_OBJECT

public:
    enum class Action { None, Open, NewChat };

    explicit ChatHistoryDialog(const QString& currentChatId,
                               QWidget* parent = nullptr);

    Action  resultAction()    const { return m_action; }
    QString selectedChatId()  const { return m_selectedId; }

    // ---- Static disk helpers ----
    static QString          chatsDir();
    static QString          chatFilePath(const QString& id);
    static QVector<ChatEntry> listChats();
    static bool             deleteChat(const QString& id);

private:
    void rebuildList();
    void onItemDoubleClicked(QListWidgetItem* item);

    QListWidget* m_list      = nullptr;
    QLabel*      m_emptyHint = nullptr;
    QString      m_currentChatId;
    QString      m_selectedId;
    Action       m_action    = Action::None;
};

#endif // CHAT_HISTORY_DIALOG_H
