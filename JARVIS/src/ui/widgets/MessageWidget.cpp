#include "MessageWidget.h"

#include <QDateTime>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

namespace {

bool    g_showTimestamps    = true;
QString g_userDisplayName   = QStringLiteral("YOU");

QString currentTimeString() {
    return QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));
}

// Light Markdown -> HTML. Intentionally minimal — handles the subset that
// chat models actually emit: ```code blocks```, `inline code`, **bold**,
// *italic*, _italic_, "- " / "* " bullets, "1. " ordered lists, blank-line
// paragraphs.
QString markdownToHtml(const QString& src) {
    // 1. Pull out fenced code blocks first so their inner contents are
    //    excluded from inline replacements.
    struct Block { QString html; };
    QList<Block> blocks;
    QString working = src;

    static const QRegularExpression fenceRe(
        QStringLiteral(R"(```(?:[^\n]*)\n([\s\S]*?)```)"));
    {
        QString out;
        int last = 0;
        QRegularExpressionMatchIterator it = fenceRe.globalMatch(working);
        while (it.hasNext()) {
            const auto m = it.next();
            out.append(working.mid(last, m.capturedStart() - last));
            const QString placeholder =
                QStringLiteral("\x01BLOCK%1\x01").arg(blocks.size());
            const QString body = m.captured(1).toHtmlEscaped();
            blocks.push_back({QStringLiteral(
                "<pre style='background:#0a1018; color:#e6edf3; "
                "padding:10px 12px; border-radius:8px; "
                "border:1px solid rgba(60,78,102,160); "
                "font-family:Consolas,\"Cascadia Mono\",monospace; "
                "font-size:12.5px; white-space:pre-wrap;'>%1</pre>").arg(body)});
            out.append(placeholder);
            last = m.capturedEnd();
        }
        out.append(working.mid(last));
        working = out;
    }

    // 2. Escape everything else, then run inline replacements.
    QString escaped = working.toHtmlEscaped();

    // Inline code `…`.
    static const QRegularExpression inlineCodeRe(QStringLiteral("`([^`\\n]+)`"));
    escaped.replace(inlineCodeRe,
        QStringLiteral("<code style='background:rgba(60,78,102,90); "
                       "padding:1px 5px; border-radius:4px; "
                       "font-family:Consolas,monospace; "
                       "font-size:12.5px;'>\\1</code>"));

    // Bold **…** (and __…__).
    static const QRegularExpression boldRe(QStringLiteral(R"(\*\*([^*\n]+)\*\*)"));
    escaped.replace(boldRe, QStringLiteral("<b>\\1</b>"));
    static const QRegularExpression boldUnderRe(QStringLiteral(R"(__([^_\n]+)__)"));
    escaped.replace(boldUnderRe, QStringLiteral("<b>\\1</b>"));

    // Italic *…* (avoid matching ** by requiring non-* on either side).
    static const QRegularExpression italicRe(
        QStringLiteral(R"((^|[^*])\*([^*\n]+)\*(?!\*))"));
    escaped.replace(italicRe, QStringLiteral("\\1<i>\\2</i>"));
    static const QRegularExpression italicUnderRe(
        QStringLiteral(R"((^|[^_])_([^_\n]+)_(?!_))"));
    escaped.replace(italicUnderRe, QStringLiteral("\\1<i>\\2</i>"));

    // 3. Reassemble: split into lines, group bullets / numbered lists into
    //    <ul>/<ol>, paragraphs separated by blank lines.
    const QStringList lines = escaped.split(QChar('\n'));
    QString out;
    enum class Mode { None, Ul, Ol };
    Mode mode = Mode::None;
    auto closeList = [&]() {
        if (mode == Mode::Ul) out += QStringLiteral("</ul>");
        if (mode == Mode::Ol) out += QStringLiteral("</ol>");
        mode = Mode::None;
    };

    static const QRegularExpression bulletRe(QStringLiteral(R"(^\s*[-*]\s+(.*)$)"));
    static const QRegularExpression numberRe(QStringLiteral(R"(^\s*\d+\.\s+(.*)$)"));

    for (const QString& raw : lines) {
        const auto bm = bulletRe.match(raw);
        const auto nm = numberRe.match(raw);
        if (bm.hasMatch()) {
            if (mode != Mode::Ul) {
                closeList();
                out += QStringLiteral("<ul style='margin:4px 0 4px 18px;'>");
                mode = Mode::Ul;
            }
            out += QStringLiteral("<li>") + bm.captured(1) + QStringLiteral("</li>");
            continue;
        }
        if (nm.hasMatch()) {
            if (mode != Mode::Ol) {
                closeList();
                out += QStringLiteral("<ol style='margin:4px 0 4px 22px;'>");
                mode = Mode::Ol;
            }
            out += QStringLiteral("<li>") + nm.captured(1) + QStringLiteral("</li>");
            continue;
        }
        // Non-list line.
        closeList();
        if (raw.trimmed().isEmpty()) {
            out += QStringLiteral("<br/>");
        } else {
            out += raw + QStringLiteral("<br/>");
        }
    }
    closeList();

    // 4. Inline the code-block placeholders back in.
    for (int i = 0; i < blocks.size(); ++i) {
        out.replace(QStringLiteral("\x01BLOCK%1\x01").arg(i), blocks[i].html);
    }
    return out;
}

} // namespace

void MessageWidget::setShowTimestamps(bool show) { g_showTimestamps = show; }
bool MessageWidget::showTimestamps()             { return g_showTimestamps; }

void MessageWidget::setUserDisplayName(const QString& name) {
    g_userDisplayName = name.isEmpty() ? QStringLiteral("YOU") : name.toUpper();
}
QString MessageWidget::userDisplayName() { return g_userDisplayName; }

MessageWidget::MessageWidget(const QString& text, bool isUser, QWidget* parent)
    : MessageWidget(text, isUser ? Kind::User : Kind::Ai, parent)
{}

MessageWidget::MessageWidget(const QString& text, Kind kind, QWidget* parent)
    : QWidget(parent),
      m_kind(kind),
      m_isUser(kind == Kind::User)
{
    setAttribute(Qt::WA_StyledBackground, false);
    m_rawText = text;
    buildUi(text);

    // Coalescing flush timer for streaming AI responses. Lazy-allocated
    // because user / system bubbles never need it.
    if (m_kind == Kind::Ai) {
        m_flushTimer = new QTimer(this);
        m_flushTimer->setSingleShot(true);
        m_flushTimer->setInterval(40);
        connect(m_flushTimer, &QTimer::timeout, this, [this]() {
            if (!m_dirty || !m_textLabel) return;
            m_dirty = false;
            m_textLabel->setText(m_rawText + QStringLiteral(" ▍"));
        });
    }

    // System bubbles & user bubbles arrive complete, so finalize them
    // immediately. AI bubbles stay "streaming" until onReplyFinished()
    // calls finalize() and runs markdown -> HTML.
    if (m_kind != Kind::Ai) finalize();
}

void MessageWidget::buildUi(const QString& text) {
    auto* outer = new QVBoxLayout(this);
    outer->setSpacing(4);
    if (m_kind == Kind::System) {
        outer->setContentsMargins(40, 4, 40, 4);
    } else {
        // Asymmetric margins keep the user vs. AI lane visually distinct
        // without leaving big empty gutters: a small inset on the "owner"
        // side, a tighter offset on the opposite side. The bubble row
        // stretchers below give the bubble most of the available width.
        outer->setContentsMargins(m_isUser ? 40 : 8, 6, m_isUser ? 8 : 40, 6);
    }

    // ---- Header: author + timestamp (skipped for system bubbles) ----
    if (m_kind != Kind::System) {
        auto* header = new QHBoxLayout;
        header->setContentsMargins(8, 0, 8, 0);
        header->setSpacing(8);

        m_authorLabel = new QLabel(
            m_isUser ? g_userDisplayName : QStringLiteral("JARVIS"), this);
        m_authorLabel->setStyleSheet(m_isUser
            ? QStringLiteral("color: #9bb6e0; font-size: 10.5px; font-weight: 700; "
                             "letter-spacing: 1.5px; text-transform: uppercase; "
                             "background: transparent;")
            : QStringLiteral("color: #58a6ff; font-size: 10.5px; font-weight: 700; "
                             "letter-spacing: 1.5px; text-transform: uppercase; "
                             "background: transparent;"));

        m_timeLabel = new QLabel(currentTimeString(), this);
        m_timeLabel->setStyleSheet(QStringLiteral(
            "color: #5e6c80; font-size: 9.5px; font-weight: 500; "
            "letter-spacing: 1px; background: transparent;"));
        m_timeLabel->setVisible(g_showTimestamps);

        if (m_isUser) {
            header->addStretch();
            header->addWidget(m_timeLabel);
            header->addWidget(m_authorLabel);
        } else {
            header->addWidget(m_authorLabel);
            header->addWidget(m_timeLabel);
            header->addStretch();
        }
        outer->addLayout(header);
    }

    // ---- Bubble: a styled QFrame holding a transparent QLabel ----
    m_bubble = new QFrame(this);

    QString bubbleQss;
    QString textQss;
    Qt::Alignment bubbleAlign = Qt::AlignLeft;
    if (m_kind == Kind::User) {
        m_bubble->setObjectName(QStringLiteral("userBubble"));
        bubbleAlign = Qt::AlignRight;
        bubbleQss = QStringLiteral(R"_(
            QFrame#userBubble {
                background: qlineargradient(
                    x1:0, y1:0, x2:1, y2:1,
                    stop:0 #2f81f7, stop:1 #1d6def);
                border: 1px solid rgba(255,255,255,40);
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
                border-bottom-left-radius: 16px;
                border-bottom-right-radius: 4px;
            })_");
        textQss = QStringLiteral(
            "color: #f4f7fb; font-size: 13.5px; background: transparent;");
    } else if (m_kind == Kind::System) {
        m_bubble->setObjectName(QStringLiteral("sysBubble"));
        bubbleAlign = Qt::AlignHCenter;
        bubbleQss = QStringLiteral(R"_(
            QFrame#sysBubble {
                background: rgba(20, 28, 40, 200);
                border: 1px dashed rgba(120, 140, 170, 110);
                border-radius: 10px;
            })_");
        textQss = QStringLiteral(
            "color: #b6c4d8; font-size: 11.5px; font-style: italic; "
            "letter-spacing: 0.3px; background: transparent;");
    } else {
        m_bubble->setObjectName(QStringLiteral("aiBubble"));
        bubbleAlign = Qt::AlignLeft;
        bubbleQss = QStringLiteral(R"_(
            QFrame#aiBubble {
                background: qlineargradient(
                    x1:0, y1:0, x2:0, y2:1,
                    stop:0 #16202a,
                    stop:1 #0d131c);
                border: 1px solid rgba(60, 78, 102, 200);
                border-top-left-radius: 16px;
                border-top-right-radius: 16px;
                border-bottom-left-radius: 4px;
                border-bottom-right-radius: 16px;
            })_");
        textQss = QStringLiteral(
            "color: #e6edf3; font-size: 13.5px; background: transparent;");
    }
    m_bubble->setAttribute(Qt::WA_StyledBackground, true);
    m_bubble->setStyleSheet(bubbleQss);

    auto* bubbleLay = new QVBoxLayout(m_bubble);
    bubbleLay->setContentsMargins(14, 11, 14, 11);
    bubbleLay->setSpacing(0);

    m_textLabel = new QLabel(text, m_bubble);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textLabel->setOpenExternalLinks(true);
    m_textLabel->setTextFormat(Qt::PlainText);
    m_textLabel->setStyleSheet(textQss);
    bubbleLay->addWidget(m_textLabel);

    // Soft drop shadow on the bubble itself. The previous black halo
    // (alpha 150) read as a dark ring on the obsidian background — we now
    // use a thin, low-alpha *accent* glow instead, which blends into the
    // gradient instead of fighting it.
    if (m_kind != Kind::System) {
        auto* shadow = new QGraphicsDropShadowEffect(m_bubble);
        shadow->setBlurRadius(18.0);
        shadow->setOffset(0, 4);
        shadow->setColor(m_isUser ? QColor(31, 110, 235, 70)
                                  : QColor(20, 60, 120, 60));
        m_bubble->setGraphicsEffect(shadow);
    }

    // Bubble row — gives the bubble most of the available width. Stretches
    // are 1 (gutter) : 7 (bubble) so the bubble visually fills ~80% of the
    // lane while still hugging its side of the chat. Word-wrap inside the
    // QLabel handles the actual line breaks naturally.
    auto* bubbleRow = new QHBoxLayout;
    bubbleRow->setContentsMargins(0, 0, 0, 0);
    if (bubbleAlign == Qt::AlignRight) {
        bubbleRow->addStretch(1);
        bubbleRow->addWidget(m_bubble, /*stretch=*/7);
    } else if (bubbleAlign == Qt::AlignHCenter) {
        bubbleRow->addStretch(1);
        bubbleRow->addWidget(m_bubble, /*stretch=*/4);
        bubbleRow->addStretch(1);
    } else {
        bubbleRow->addWidget(m_bubble, /*stretch=*/7);
        bubbleRow->addStretch(1);
    }
    outer->addLayout(bubbleRow);
}

void MessageWidget::appendText(const QString& text) {
    if (!m_textLabel) return;
    m_finalized = false;
    m_rawText.append(text);

    // For non-AI bubbles (created complete) just push immediately.
    if (m_kind != Kind::Ai || !m_flushTimer) {
        m_textLabel->setTextFormat(Qt::PlainText);
        m_textLabel->setText(m_rawText);
        return;
    }

    // Streaming AI bubble: coalesce token bursts into ~40 ms updates so we
    // don't kick a QLabel re-flow on every token. The first token paints
    // immediately for snappy feel; subsequent ones wait for the timer.
    m_textLabel->setTextFormat(Qt::PlainText);
    m_dirty = true;
    if (!m_flushTimer->isActive()) {
        m_flushTimer->start();
    }
    if (m_timeLabel) m_timeLabel->setText(currentTimeString());
}

void MessageWidget::finalize() {
    if (!m_textLabel) return;
    if (m_finalized) return;
    m_finalized = true;
    if (m_flushTimer && m_flushTimer->isActive()) m_flushTimer->stop();
    m_dirty = false;

    if (m_kind == Kind::Ai) {
        // Run light markdown -> HTML and switch to RichText.
        const QString html = markdownToHtml(m_rawText);
        m_textLabel->setTextFormat(Qt::RichText);
        m_textLabel->setText(html);
    } else {
        // User / System bubbles stay plain text — safe + selectable.
        m_textLabel->setTextFormat(Qt::PlainText);
        m_textLabel->setText(m_rawText);
    }
    if (m_timeLabel) m_timeLabel->setText(currentTimeString());
}
