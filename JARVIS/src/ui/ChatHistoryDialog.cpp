#include "ChatHistoryDialog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

QString humanWhen(const QDateTime& dt) {
    if (!dt.isValid()) return QStringLiteral("—");
    const qint64 secs = dt.secsTo(QDateTime::currentDateTime());
    if (secs < 60)        return QStringLiteral("щойно");
    if (secs < 3600)      return QString(QStringLiteral("%1 хв тому")).arg(secs / 60);
    if (secs < 86400)     return QString(QStringLiteral("%1 год тому")).arg(secs / 3600);
    if (secs < 2 * 86400) return QStringLiteral("вчора");
    if (secs < 7 * 86400) return QString(QStringLiteral("%1 дн тому")).arg(secs / 86400);
    return dt.toString(QStringLiteral("dd MMM yyyy"));
}

} // namespace

// =============================================================================
//  Static disk helpers
// =============================================================================

QString ChatHistoryDialog::chatsDir() {
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/chats");
    QDir().mkpath(dir);
    return dir;
}

QString ChatHistoryDialog::chatFilePath(const QString& id) {
    return chatsDir() + QStringLiteral("/") + id + QStringLiteral(".json");
}

QVector<ChatEntry> ChatHistoryDialog::listChats() {
    QVector<ChatEntry> out;
    QDir d(chatsDir());
    const QFileInfoList files = d.entryInfoList(
        QStringList{ QStringLiteral("*.json") }, QDir::Files,
        QDir::Time | QDir::Reversed);
    for (const QFileInfo& fi : files) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (!doc.isObject()) continue;

        const QJsonObject obj = doc.object();
        ChatEntry e;
        e.id        = fi.completeBaseName();
        e.title     = obj.value(QStringLiteral("title")).toString(
            QStringLiteral("Без назви"));
        e.updatedAt = QDateTime::fromString(
            obj.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);
        if (!e.updatedAt.isValid()) e.updatedAt = fi.lastModified();
        e.messageCount = obj.value(QStringLiteral("messages")).toArray().size();
        out.push_back(e);
    }
    // Newest first.
    std::sort(out.begin(), out.end(), [](const ChatEntry& a, const ChatEntry& b) {
        return a.updatedAt > b.updatedAt;
    });
    return out;
}

bool ChatHistoryDialog::deleteChat(const QString& id) {
    return QFile::remove(chatFilePath(id));
}

// =============================================================================
//  Dialog
// =============================================================================

ChatHistoryDialog::ChatHistoryDialog(const QString& currentChatId, QWidget* parent)
    : QDialog(parent),
      m_currentChatId(currentChatId)
{
    setWindowTitle(QStringLiteral("JARVIS · Історія чатів"));
    setModal(true);
    resize(560, 620);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 22, 18);
    root->setSpacing(14);

    auto* eyebrow = new QLabel(QStringLiteral("JARVIS · АРХІВ"), this);
    eyebrow->setStyleSheet(QStringLiteral(
        "color: #58a6ff; font-size: 10px; font-weight: 800; letter-spacing: 4px;"));
    auto* title = new QLabel(QStringLiteral("Історія розмов"), this);
    title->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 22px; font-weight: 800; letter-spacing: 0.3px;"));
    auto* subtitle = new QLabel(
        QStringLiteral("Усі попередні чати збережено локально на твоєму ПК. "
                       "Подвійний клік відкриває чат, ⌫ видаляє його."), this);
    subtitle->setStyleSheet(QStringLiteral(
        "color: #8a99b1; font-size: 12px;"));
    subtitle->setWordWrap(true);

    root->addWidget(eyebrow);
    root->addWidget(title);
    root->addWidget(subtitle);

    // ---- List ----
    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("chatList"));
    m_list->setAlternatingRowColors(false);
    m_list->setUniformItemSizes(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    m_emptyHint = new QLabel(
        QStringLiteral("Поки що немає збережених чатів.\n"
                       "Створи перший — натисни «Новий чат»."), this);
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setStyleSheet(QStringLiteral(
        "color: #6b7a90; font-size: 12.5px;"));
    m_emptyHint->setWordWrap(true);

    root->addWidget(m_list, /*stretch=*/1);
    root->addWidget(m_emptyHint);

    // ---- Buttons ----
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);

    auto* newBtn    = new QPushButton(QStringLiteral("＋  Новий чат"), this);
    auto* deleteBtn = new QPushButton(QStringLiteral("Видалити"), this);
    auto* openBtn   = new QPushButton(QStringLiteral("Відкрити"), this);
    auto* cancelBtn = new QPushButton(QStringLiteral("Закрити"), this);

    newBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setDefault(true);

    btnRow->addWidget(newBtn);
    btnRow->addStretch();
    btnRow->addWidget(deleteBtn);
    btnRow->addWidget(openBtn);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    connect(newBtn, &QPushButton::clicked, this, [this]() {
        m_action = Action::NewChat;
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem* it = m_list->currentItem();
        if (!it) return;
        m_selectedId = it->data(Qt::UserRole).toString();
        m_action = Action::Open;
        accept();
    });
    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem* it = m_list->currentItem();
        if (!it) return;
        const QString id = it->data(Qt::UserRole).toString();
        if (id.isEmpty()) return;
        deleteChat(id);
        rebuildList();
    });
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &ChatHistoryDialog::onItemDoubleClicked);

    // ---- Stylesheet ----
    setStyleSheet(QStringLiteral(R"_(
        QDialog { background-color: #06090d; color: #e6edf3; }
        QListWidget#chatList {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #08111c, stop:1 #060a12);
            border: 1px solid #182334;
            border-radius: 12px;
            padding: 6px;
            outline: 0;
        }
        QListWidget#chatList::item {
            background: rgba(20, 28, 40, 200);
            color: #e6edf3;
            border: 1px solid rgba(60, 78, 102, 100);
            border-radius: 10px;
            padding: 12px 14px;
            margin: 4px 2px;
        }
        QListWidget#chatList::item:hover {
            border-color: rgba(47, 129, 247, 200);
            background: rgba(28, 40, 60, 230);
        }
        QListWidget#chatList::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(47, 129, 247, 80),
                stop:1 rgba(20, 30, 50, 230));
            border: 1px solid #2f81f7;
            color: #ffffff;
        }
        QPushButton {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1, stop:0 #182336, stop:1 #0e1626);
            color: #e6edf3;
            border: 1px solid #2a3a52;
            border-radius: 10px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover { border-color: #2f81f7; color: #ffffff; }
        QPushButton:default {
            background-color: qlineargradient(
                x1:0, y1:0, x2:0, y2:1, stop:0 #2f81f7, stop:1 #1d6def);
            border: 1px solid rgba(255,255,255,40);
            color: #ffffff;
        }
    )_"));

    rebuildList();
}

void ChatHistoryDialog::rebuildList() {
    m_list->clear();

    const QVector<ChatEntry> entries = listChats();
    m_emptyHint->setVisible(entries.isEmpty());
    m_list->setVisible(!entries.isEmpty());

    for (const ChatEntry& e : entries) {
        auto* it = new QListWidgetItem;
        const QString currentTag = (e.id == m_currentChatId)
            ? QStringLiteral("  <b style='color:#58a6ff;'>· поточний</b>")
            : QString();
        // Item label uses Qt's "two-line" pattern via a single string with newline.
        QString display = e.title.trimmed();
        if (display.isEmpty()) display = QStringLiteral("Без назви");
        if (display.length() > 80) display = display.left(78) + QStringLiteral("…");

        const QString meta = QString(QStringLiteral("%1   ·   %2 повідомлень"))
                                 .arg(humanWhen(e.updatedAt))
                                 .arg(e.messageCount);
        it->setText(display + QStringLiteral("\n") + meta);
        it->setData(Qt::UserRole, e.id);
        it->setSizeHint(QSize(0, 56));
        if (e.id == m_currentChatId) {
            QFont f = it->font();
            f.setBold(true);
            it->setFont(f);
        }
        m_list->addItem(it);
        Q_UNUSED(currentTag);
    }

    if (m_list->count() > 0) m_list->setCurrentRow(0);
}

void ChatHistoryDialog::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    m_selectedId = item->data(Qt::UserRole).toString();
    m_action = Action::Open;
    accept();
}
