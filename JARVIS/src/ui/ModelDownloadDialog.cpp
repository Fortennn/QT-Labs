#include "ModelDownloadDialog.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString humanBytes(qint64 b) {
    if (b <= 0) return QStringLiteral("?");
    static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    double v = static_cast<double>(b);
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    return QString::asprintf("%.1f %s", v, units[u]);
}

} // namespace

QVector<ModelDownloadDialog::CatalogEntry>
ModelDownloadDialog::builtinCatalog() {
    // Кураторний список перевірених «починаючих» GGUF-моделей з HuggingFace.
    // Усі прямі посилання на resolve/main, без проміжного API. Ім'я файлу
    // використовується як ціль збереження — суфікс .gguf обов'язковий.
    return {
        {
            QStringLiteral("Qwen 2.5 0.5B Instruct (Q4_K_M) — найшвидша, ~400 MB"),
            QStringLiteral("qwen2.5-0.5b-instruct-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/"
                           "resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf?download=true"),
            400LL * 1024 * 1024,
            QStringLiteral("Дуже маленька (500M параметрів). Запускається на будь-якому ПК. "
                           "Підходить для тестування інтерфейсу, не для серйозних діалогів. "
                           "RAM: ≥2 GB. Шаблон: ChatML.")
        },
        {
            QStringLiteral("Qwen 2.5 1.5B Instruct (Q4_K_M) — оптимальна, ~1 GB"),
            QStringLiteral("qwen2.5-1.5b-instruct-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/"
                           "resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf?download=true"),
            1100LL * 1024 * 1024,
            QStringLiteral("Хороший баланс швидкості та якості. Працює на CPU. "
                           "RAM: ≥4 GB. Шаблон: ChatML.")
        },
        {
            QStringLiteral("Llama 3.2 1B Instruct (Q4_K_M) — ~800 MB"),
            QStringLiteral("llama-3.2-1b-instruct-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/"
                           "resolve/main/Llama-3.2-1B-Instruct-Q4_K_M.gguf?download=true"),
            800LL * 1024 * 1024,
            QStringLiteral("Від Meta. Швидка, добре розуміє інструкції. "
                           "RAM: ≥3 GB. Шаблон: Llama 3.")
        },
        {
            QStringLiteral("Llama 3.2 3B Instruct (Q4_K_M) — ~2 GB"),
            QStringLiteral("llama-3.2-3b-instruct-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/"
                           "resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf?download=true"),
            2100LL * 1024 * 1024,
            QStringLiteral("Краща розмова за 1B-версію. Потребує більше RAM/часу. "
                           "RAM: ≥6 GB. Шаблон: Llama 3.")
        },
        {
            QStringLiteral("Phi-3.5 Mini 3.8B Instruct (Q4_K_M) — ~2.3 GB"),
            QStringLiteral("phi-3.5-mini-instruct-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/bartowski/Phi-3.5-mini-instruct-GGUF/"
                           "resolve/main/Phi-3.5-mini-instruct-Q4_K_M.gguf?download=true"),
            2300LL * 1024 * 1024,
            QStringLiteral("Від Microsoft. Сильна в коді й логіці. "
                           "RAM: ≥6 GB. Шаблон: ChatML.")
        },
        {
            QStringLiteral("Dolphin 3.0 Llama-3.1 8B (Q4_K_M) — ~4.9 GB"),
            QStringLiteral("dolphin-3.0-llama3.1-8b-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/bartowski/Dolphin3.0-Llama3.1-8B-GGUF/"
                           "resolve/main/Dolphin3.0-Llama3.1-8B-Q4_K_M.gguf?download=true"),
            4900LL * 1024 * 1024,
            QStringLiteral("Без цензури, гарно дотримується інструкцій. "
                           "RAM: ≥10 GB або GPU з ≥6 GB VRAM. Шаблон: ChatML.")
        },
        {
            QStringLiteral("Mistral 7B Instruct v0.3 (Q4_K_M) — ~4.4 GB"),
            QStringLiteral("mistral-7b-instruct-v0.3-q4_k_m.gguf"),
            QStringLiteral("https://huggingface.co/bartowski/Mistral-7B-Instruct-v0.3-GGUF/"
                           "resolve/main/Mistral-7B-Instruct-v0.3-Q4_K_M.gguf?download=true"),
            4400LL * 1024 * 1024,
            QStringLiteral("Класична 7B-модель. RAM: ≥8 GB. Шаблон: Mistral.")
        },
    };
}

QString ModelDownloadDialog::defaultDownloadDir() {
    const QString primary = QDir(QCoreApplication::applicationDirPath())
                                .absoluteFilePath(QStringLiteral("models"));
    QDir d(primary);
    if (!d.exists()) {
        QDir().mkpath(primary);
    }
    // Перевіряємо writability через створення тимчасового файла.
    QFile probe(primary + QStringLiteral("/.write-probe"));
    if (probe.open(QIODevice::WriteOnly)) {
        probe.close();
        probe.remove();
        return primary;
    }
    // Fallback: AppData / XDG.
    const QString appData = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    const QString fallback = appData + QStringLiteral("/models");
    QDir().mkpath(fallback);
    return fallback;
}

ModelDownloadDialog::ModelDownloadDialog(QWidget* parent)
    : QDialog(parent),
      m_net(new QNetworkAccessManager(this))
{
    setWindowTitle(QStringLiteral("Завантаження моделі"));
    setModal(true);
    resize(640, 360);
    buildUi();
    onCatalogChanged(m_catalog->currentIndex());
}

void ModelDownloadDialog::buildUi() {
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(18, 18, 18, 14);
    main->setSpacing(12);

    auto* title = new QLabel(QStringLiteral(
        "Обери модель зі списку або встав свій прямий URL до .gguf-файла "
        "(наприклад, з HuggingFace «resolve/main»)."), this);
    title->setWordWrap(true);
    title->setStyleSheet(QStringLiteral("color: #c0d0e8;"));
    main->addWidget(title);

    // ---- Кураторний каталог ----
    auto* catRow = new QHBoxLayout;
    auto* catLbl = new QLabel(QStringLiteral("Каталог:"), this);
    catLbl->setMinimumWidth(80);
    m_catalog = new QComboBox(this);
    for (const auto& e : builtinCatalog()) {
        m_catalog->addItem(e.name);
    }
    m_catalog->addItem(QStringLiteral("(Власний URL…)"));
    catRow->addWidget(catLbl);
    catRow->addWidget(m_catalog, 1);
    main->addLayout(catRow);

    m_descLabel = new QLabel(this);
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet(QStringLiteral(
        "color: #8a99b1; font-size: 12px; "
        "padding: 8px 10px; "
        "background: rgba(20, 28, 40, 200); "
        "border: 1px solid rgba(60, 78, 102, 140); "
        "border-radius: 8px;"));
    main->addWidget(m_descLabel);

    // ---- URL / ім'я файлу ----
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    auto* urlLbl = new QLabel(QStringLiteral("URL:"), this);
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(QStringLiteral("https://.../something.gguf"));
    grid->addWidget(urlLbl,     0, 0);
    grid->addWidget(m_urlEdit,  0, 1);

    auto* nameLbl = new QLabel(QStringLiteral("Назва файла:"), this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(QStringLiteral("model.gguf"));
    grid->addWidget(nameLbl,    1, 0);
    grid->addWidget(m_nameEdit, 1, 1);

    main->addLayout(grid);

    // ---- Прогрес ----
    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    main->addWidget(m_progress);

    m_statusLabel = new QLabel(QStringLiteral("Готово."), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8a99b1;"));
    main->addWidget(m_statusLabel);

    main->addStretch();

    // ---- Кнопки ----
    auto* btnRow = new QHBoxLayout;
    m_startBtn  = new QPushButton(QStringLiteral("Завантажити"), this);
    m_cancelBtn = new QPushButton(QStringLiteral("Скасувати"),   this);
    m_closeBtn  = new QPushButton(QStringLiteral("Закрити"),     this);
    m_cancelBtn->setEnabled(false);
    btnRow->addStretch();
    btnRow->addWidget(m_startBtn);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_closeBtn);
    main->addLayout(btnRow);

    connect(m_catalog,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ModelDownloadDialog::onCatalogChanged);
    connect(m_startBtn,  &QPushButton::clicked,
            this, &ModelDownloadDialog::onStart);
    connect(m_cancelBtn, &QPushButton::clicked,
            this, &ModelDownloadDialog::onCancel);
    connect(m_closeBtn,  &QPushButton::clicked, this, &QDialog::accept);
}

void ModelDownloadDialog::onCatalogChanged(int idx) {
    const auto cat = builtinCatalog();
    if (idx >= 0 && idx < cat.size()) {
        const auto& e = cat[idx];
        m_urlEdit->setText(e.url);
        m_nameEdit->setText(e.fileName);
        m_descLabel->setText(
            QStringLiteral("%1\n\nПриблизний розмір: %2")
                .arg(e.description, humanBytes(e.approxSize)));
    } else {
        m_urlEdit->clear();
        m_nameEdit->clear();
        m_descLabel->setText(QStringLiteral(
            "Введи будь-яке пряме посилання на .gguf-файл. "
            "Це може бути HuggingFace «resolve/main», власний сервер чи "
            "локальний дзеркало."));
    }
}

void ModelDownloadDialog::resetUi() {
    m_progress->setValue(0);
    m_statusLabel->setText(QStringLiteral("Готово."));
    m_startBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_catalog->setEnabled(true);
    m_urlEdit->setEnabled(true);
    m_nameEdit->setEnabled(true);
}

void ModelDownloadDialog::cancelActive() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_outFile) {
        m_outFile->close();
        m_outFile->remove();   // не залишаємо часткові файли
        delete m_outFile;
        m_outFile = nullptr;
    }
    m_currentPath.clear();
}

void ModelDownloadDialog::onStart() {
    const QString url  = m_urlEdit->text().trimmed();
    QString name = m_nameEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, windowTitle(),
            QStringLiteral("Введи URL для завантаження."));
        return;
    }
    if (name.isEmpty()) {
        name = QFileInfo(QUrl(url).path()).fileName();
        if (name.isEmpty()) name = QStringLiteral("model.gguf");
    }
    if (!name.endsWith(QLatin1String(".gguf"), Qt::CaseInsensitive)) {
        name += QStringLiteral(".gguf");
    }

    const QString dir = defaultDownloadDir();
    m_currentPath = dir + QLatin1Char('/') + name;

    if (QFileInfo::exists(m_currentPath)) {
        const auto r = QMessageBox::question(this, windowTitle(),
            QStringLiteral("Файл %1 вже існує. Перезаписати?")
                .arg(QDir::toNativeSeparators(m_currentPath)));
        if (r != QMessageBox::Yes) return;
        QFile::remove(m_currentPath);
    }

    m_outFile = new QFile(m_currentPath);
    if (!m_outFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, windowTitle(),
            QStringLiteral("Не вдається записати у %1.\n%2")
                .arg(QDir::toNativeSeparators(m_currentPath),
                     m_outFile->errorString()));
        delete m_outFile;
        m_outFile = nullptr;
        return;
    }

    QNetworkRequest req(QUrl{url});
    // Qt 6 dropped the deprecated FollowRedirectsAttribute; RedirectPolicy
    // is the supported API. HuggingFace returns a 302 to a CDN.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent", "JARVIS-Downloader/1.0");
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/octet-stream"));

    m_reply = m_net->get(req);
    m_lastReceived = 0;
    m_lastTimeMs = QDateTime::currentMSecsSinceEpoch();

    connect(m_reply, &QNetworkReply::downloadProgress,
            this, &ModelDownloadDialog::onProgress);
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_reply && m_outFile)
            m_outFile->write(m_reply->readAll());
    });
    connect(m_reply, &QNetworkReply::finished,
            this, &ModelDownloadDialog::onFinished);
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &ModelDownloadDialog::onError);

    m_startBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_catalog->setEnabled(false);
    m_urlEdit->setEnabled(false);
    m_nameEdit->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("Завантажую → %1")
                               .arg(QDir::toNativeSeparators(m_currentPath)));
}

void ModelDownloadDialog::onCancel() {
    cancelActive();
    m_statusLabel->setText(QStringLiteral("Скасовано користувачем."));
    resetUi();
}

void ModelDownloadDialog::onProgress(qint64 received, qint64 total) {
    if (total > 0) {
        m_progress->setRange(0, 100);
        m_progress->setValue(static_cast<int>(received * 100 / total));
    } else {
        m_progress->setRange(0, 0);
    }

    // Швидкість: різниця в байтах за останній інтервал.
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 dt = nowMs - m_lastTimeMs;
    if (dt > 500) {
        const qint64 db = received - m_lastReceived;
        const double speed = db * 1000.0 / dt;
        m_lastTimeMs   = nowMs;
        m_lastReceived = received;
        m_statusLabel->setText(QStringLiteral("Прийнято %1 з %2 — %3/c")
            .arg(humanBytes(received),
                 total > 0 ? humanBytes(total) : QStringLiteral("?"),
                 humanBytes(static_cast<qint64>(speed))));
    }
}

void ModelDownloadDialog::onFinished() {
    if (!m_reply) return;
    const bool ok = (m_reply->error() == QNetworkReply::NoError);
    if (m_outFile) {
        if (m_reply->bytesAvailable() > 0) {
            m_outFile->write(m_reply->readAll());
        }
        m_outFile->flush();
        m_outFile->close();
    }
    m_reply->deleteLater();
    m_reply = nullptr;
    if (!ok) {
        m_statusLabel->setText(QStringLiteral("Помилка завантаження."));
        if (m_outFile) {
            m_outFile->remove();
            delete m_outFile;
            m_outFile = nullptr;
        }
        resetUi();
        return;
    }
    if (m_outFile) {
        delete m_outFile;
        m_outFile = nullptr;
    }
    m_downloadedPath = m_currentPath;
    m_statusLabel->setText(QStringLiteral("Готово: %1")
                               .arg(QDir::toNativeSeparators(m_currentPath)));
    m_progress->setRange(0, 100);
    m_progress->setValue(100);
    resetUi();
    emit modelDownloaded(m_downloadedPath);
}

void ModelDownloadDialog::onError() {
    if (!m_reply) return;
    const QString msg = m_reply->errorString();
    qWarning() << "[JARVIS DL] error:" << msg;
    m_statusLabel->setText(QStringLiteral("Помилка: %1").arg(msg));
}
