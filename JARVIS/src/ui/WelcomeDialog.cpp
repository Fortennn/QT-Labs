#include "WelcomeDialog.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

// Center-square-crop a QImage to its smallest dimension.
QImage cropToSquare(const QImage& src) {
    if (src.isNull()) return src;
    const int side = std::min(src.width(), src.height());
    const int x    = (src.width()  - side) / 2;
    const int y    = (src.height() - side) / 2;
    return src.copy(x, y, side, side);
}

// Round-mask a square pixmap. Caller passes a square pixmap; we return one
// where outside-the-circle pixels are transparent.
QPixmap circularMask(const QPixmap& squarePix) {
    if (squarePix.isNull()) return squarePix;

    const int side = squarePix.width();
    QPixmap rounded(side, side);
    rounded.fill(Qt::transparent);

    QPainter p(&rounded);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addEllipse(0, 0, side, side);
    p.setClipPath(path);
    p.drawPixmap(0, 0, squarePix);
    return rounded;
}

} // namespace

// =============================================================================
//  WelcomeDialog
// =============================================================================

WelcomeDialog::WelcomeDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Привіт від JARVIS"));
    setModal(true);
    setFixedSize(540, 460);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 28);

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("welcomeCard"));
    card->setStyleSheet(QStringLiteral(R"_(
        QFrame#welcomeCard {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0e1726, stop:1 #060a12);
            border: 1px solid rgba(80, 130, 200, 90);
            border-radius: 22px;
        }
    )_"));

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(60);
    shadow->setOffset(0, 16);
    shadow->setColor(QColor(0, 0, 0, 200));
    card->setGraphicsEffect(shadow);

    auto* col = new QVBoxLayout(card);
    col->setContentsMargins(36, 28, 36, 24);
    col->setSpacing(12);

    auto* eyebrow = new QLabel(QStringLiteral("JARVIS · ВІТАЮ"), card);
    eyebrow->setStyleSheet(QStringLiteral(
        "color: #58a6ff; font-size: 10.5px; font-weight: 800; letter-spacing: 4px;"));
    eyebrow->setAlignment(Qt::AlignHCenter);

    // ---- Clickable round avatar preview ---------------------------------
    m_avatarPreview = new QLabel(card);
    m_avatarPreview->setFixedSize(96, 96);
    m_avatarPreview->setAlignment(Qt::AlignCenter);
    m_avatarPreview->setCursor(Qt::PointingHandCursor);
    m_avatarPreview->setToolTip(QStringLiteral("Натисни, щоб обрати фото"));
    m_avatarPreview->installEventFilter(this); // not used; we use mousePress via subclass-less hack below
    refreshAvatarPreview();

    // Click anywhere on the avatar preview opens a file picker. Easiest
    // path without subclassing: install a click-to-open shortcut using a
    // small invisible QPushButton overlay.
    auto* avatarBtn = new QPushButton(card);
    avatarBtn->setFixedSize(96, 96);
    avatarBtn->setCursor(Qt::PointingHandCursor);
    avatarBtn->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    connect(avatarBtn, &QPushButton::clicked,
            this, &WelcomeDialog::pickAvatarInteractive);

    // Stack avatarBtn over m_avatarPreview using a single-row QHBoxLayout
    // wrapper. Simpler: place them in a QFrame with a manual stacked layout.
    auto* avatarStack = new QFrame(card);
    avatarStack->setFixedSize(96, 96);
    m_avatarPreview->setParent(avatarStack);
    avatarBtn->setParent(avatarStack);
    m_avatarPreview->move(0, 0);
    avatarBtn->move(0, 0);

    auto* avatarRow = new QHBoxLayout;
    avatarRow->setContentsMargins(0, 0, 0, 0);
    avatarRow->addStretch();
    avatarRow->addWidget(avatarStack);
    avatarRow->addStretch();

    auto* avatarHint = new QLabel(
        QStringLiteral("Натисни на коло, щоб додати фото"), card);
    avatarHint->setStyleSheet(QStringLiteral(
        "color: #6b7a90; font-size: 11px; letter-spacing: 0.5px;"));
    avatarHint->setAlignment(Qt::AlignHCenter);

    // ---- Title + subtitle ----
    auto* title = new QLabel(QStringLiteral("Як до тебе звертатись?"), card);
    title->setStyleSheet(QStringLiteral(
        "color: #ffffff; font-size: 22px; font-weight: 800; letter-spacing: 0.3px;"));
    title->setAlignment(Qt::AlignHCenter);
    title->setWordWrap(true);

    auto* subtitle = new QLabel(
        QStringLiteral("Це ім'я з'являтиметься поряд із твоїми повідомленнями. "
                       "Завжди можна змінити в Налаштуваннях."), card);
    subtitle->setStyleSheet(QStringLiteral(
        "color: #8a99b1; font-size: 12px; line-height: 1.45;"));
    subtitle->setWordWrap(true);
    subtitle->setAlignment(Qt::AlignHCenter);

    m_nameEdit = new QLineEdit(card);
    m_nameEdit->setPlaceholderText(QStringLiteral("Наприклад: Олександр"));
    m_nameEdit->setMaxLength(40);
    m_nameEdit->setText(savedName());
    m_nameEdit->setStyleSheet(QStringLiteral(R"_(
        QLineEdit {
            background: rgba(13, 19, 28, 220);
            color: #e6edf3;
            border: 1px solid #233248;
            border-radius: 12px;
            padding: 12px 16px;
            font-size: 15px;
            font-weight: 600;
            selection-background-color: #2f81f7;
        }
        QLineEdit:focus { border: 1px solid #2f81f7; }
    )_"));

    // ---- Continue + Skip buttons ----
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    auto* skipBtn = new QPushButton(QStringLiteral("Пропустити"), card);
    skipBtn->setCursor(Qt::PointingHandCursor);
    skipBtn->setStyleSheet(QStringLiteral(R"_(
        QPushButton {
            background: transparent;
            color: #8a99b1;
            border: 1px solid #233248;
            border-radius: 10px;
            padding: 10px 18px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover { color: #e6edf3; border-color: #58a6ff; }
    )_"));

    auto* okBtn = new QPushButton(QStringLiteral("Продовжити  →"), card);
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setDefault(true);
    okBtn->setStyleSheet(QStringLiteral(R"_(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f81f7, stop:1 #1d6def);
            color: #ffffff;
            border: 1px solid rgba(255,255,255,40);
            border-radius: 10px;
            padding: 10px 22px;
            font-size: 13.5px;
            font-weight: 800;
            letter-spacing: 0.3px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4593f9, stop:1 #2f81f7);
        }
    )_"));

    btnRow->addWidget(skipBtn);
    btnRow->addWidget(okBtn);

    col->addWidget(eyebrow);
    col->addLayout(avatarRow);
    col->addWidget(avatarHint);
    col->addSpacing(4);
    col->addWidget(title);
    col->addWidget(subtitle);
    col->addSpacing(2);
    col->addWidget(m_nameEdit);
    col->addStretch();
    col->addLayout(btnRow);

    root->addWidget(card);

    connect(okBtn,   &QPushButton::clicked, this, &QDialog::accept);
    connect(skipBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &QDialog::accept);

    m_nameEdit->setFocus();
}

QString WelcomeDialog::chosenName() const {
    return m_nameEdit ? m_nameEdit->text().trimmed() : QString();
}

void WelcomeDialog::pickAvatarInteractive() {
    const QString src = QFileDialog::getOpenFileName(
        this, QStringLiteral("Обрати фото"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        QStringLiteral("Зображення (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (src.isEmpty()) return;

    if (persistAvatarFromFile(src)) {
        refreshAvatarPreview();
    }
}

void WelcomeDialog::refreshAvatarPreview() {
    if (!m_avatarPreview) return;

    const int side = m_avatarPreview->width();
    QPixmap saved = savedAvatar(side);
    if (!saved.isNull()) {
        // Round-mask + halo + ring are baked into the bitmap itself so we
        // don't need any QSS border on the QLabel (which would render as a
        // square outside the circle).
        m_avatarPreview->setPixmap(roundAvatar(saved, side));
        m_avatarPreview->setStyleSheet(QStringLiteral(
            "background: transparent; border: none;"));
        m_avatarPreview->setText(QString());
    } else {
        m_avatarPreview->setPixmap(QPixmap());
        m_avatarPreview->setText(QStringLiteral("+"));
        m_avatarPreview->setStyleSheet(QStringLiteral(
            "color: #58a6ff; font-size: 32px; font-weight: 300; "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
            "  stop:0 rgba(47,129,247,55), stop:1 rgba(31,110,235,30)); "
            "border: 2px dashed rgba(47, 129, 247, 160); "
            "border-radius: %1px;").arg(side / 2));
    }
}

// =============================================================================
//  Static profile helpers
// =============================================================================

QString WelcomeDialog::savedName() {
    QSettings s;
    return s.value(QStringLiteral("user/name")).toString().trimmed();
}

void WelcomeDialog::persistName(const QString& name) {
    QSettings s;
    s.setValue(QStringLiteral("user/name"), name.trimmed());
}

QString WelcomeDialog::avatarFilePath() {
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/avatar.png");
}

QPixmap WelcomeDialog::savedAvatar(int size) {
    const QString p = avatarFilePath();
    if (!QFile::exists(p)) return QPixmap();

    QPixmap pix(p);
    if (pix.isNull()) return pix;
    if (size > 0) {
        pix = pix.scaled(size, size,
                         Qt::KeepAspectRatioByExpanding,
                         Qt::SmoothTransformation);
    }
    return pix;
}

QPixmap WelcomeDialog::roundAvatar(const QPixmap& source, int size) {
    if (size <= 0) size = 64;

    // Always render at 2× and downscale, so the antialiased ring + edge
    // stay crisp on Hi-DPI displays.
    const int hi = size * 2;

    QPixmap canvas(hi, hi);
    canvas.fill(Qt::transparent);

    QPainter p(&canvas);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!source.isNull()) {
        // Center-crop + scale the source to a square of `hi` px before clipping.
        QPixmap scaled = source.scaled(hi, hi,
                                       Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation);
        const int dx = (scaled.width()  - hi) / 2;
        const int dy = (scaled.height() - hi) / 2;

        QPainterPath clip;
        clip.addEllipse(0, 0, hi, hi);
        p.save();
        p.setClipPath(clip);
        p.drawPixmap(-dx, -dy, scaled);
        p.restore();
    } else {
        // Subtle dark plate behind the (missing) photo.
        QPainterPath clip;
        clip.addEllipse(0, 0, hi, hi);
        p.save();
        p.setClipPath(clip);
        p.fillRect(0, 0, hi, hi, QColor(0x10, 0x18, 0x24));
        p.restore();
    }

    // Soft halo + crisp inner ring, both inset so they don't get clipped.
    {
        QPen pen;
        pen.setColor(QColor(255, 255, 255, 28));
        pen.setWidthF(hi * 0.045);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        const qreal halo = pen.widthF() / 2.0;
        p.drawEllipse(QRectF(halo, halo, hi - 2 * halo, hi - 2 * halo));
    }
    {
        QPen pen;
        pen.setColor(QColor(255, 255, 255, 90));
        pen.setWidthF(hi * 0.018);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        const qreal r = hi * 0.5;
        const qreal inset = pen.widthF() * 1.4;
        p.drawEllipse(QPointF(r, r), r - inset, r - inset);
    }
    p.end();

    return canvas.scaled(size, size,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation);
}

bool WelcomeDialog::persistAvatarFromFile(const QString& sourcePath) {
    QImageReader reader(sourcePath);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) return false;

    img = cropToSquare(img);
    img = img.scaled(256, 256,
                     Qt::IgnoreAspectRatio,
                     Qt::SmoothTransformation);

    return img.save(avatarFilePath(), "PNG");
}

void WelcomeDialog::clearAvatar() {
    QFile::remove(avatarFilePath());
}
