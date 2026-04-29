#include "ParticleBackground.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QResizeEvent>

#include <cmath>

namespace {

constexpr qreal kFrameSeconds = 1.0 / 60.0;

QColor mixWithAccent(const QColor& accent, qreal hueShift, int alpha) {
    QColor c = accent.toHsv();
    qreal h = c.hsvHueF();
    if (h < 0) h = 0.6; // fallback for greys
    h = std::fmod(h + hueShift + 1.0, 1.0);
    QColor out = QColor::fromHsvF(h,
                                  qBound(0.0, c.hsvSaturationF() * 0.9, 1.0),
                                  qBound(0.0, c.valueF(),        1.0));
    out.setAlpha(alpha);
    return out;
}

} // namespace

ParticleBackground::ParticleBackground(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);

    auto* rng = QRandomGenerator::global();

    // Four soft aurora "lights". Hues are derived from the accent so changing
    // accent recolors everything coherently.
    m_blobs.reserve(4);
    for (int i = 0; i < 4; ++i) {
        Blob b;
        b.ax     = 0.35 + rng->generateDouble() * 0.20;
        b.ay     = 0.30 + rng->generateDouble() * 0.20;
        b.wx     = 0.05 + rng->generateDouble() * 0.06;
        b.wy     = 0.04 + rng->generateDouble() * 0.06;
        b.px     = rng->generateDouble() * 6.2831;
        b.py     = rng->generateDouble() * 6.2831;
        b.radius = 0.55 + rng->generateDouble() * 0.30;
        b.alpha  = 0.30 + rng->generateDouble() * 0.18;
        b.color  = mixWithAccent(m_accent,
                                 -0.06 + 0.12 * (i / 3.0), // -0.06 .. +0.06 hue
                                 230);
        m_blobs.append(b);
    }

    rebuildStars();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ParticleBackground::tick);
    m_timer->start(16); // ~60 FPS
}

void ParticleBackground::setAccentColor(const QColor& accent) {
    m_accent = accent;
    for (int i = 0; i < m_blobs.size(); ++i) {
        m_blobs[i].color = mixWithAccent(accent,
                                         -0.06 + 0.12 * (i / qMax(1, m_blobs.size() - 1)),
                                         230);
    }
    update();
}

void ParticleBackground::tick() {
    m_time += kFrameSeconds;
    update();
}

void ParticleBackground::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    rebuildStars();
}

void ParticleBackground::rebuildStars() {
    m_stars.clear();
    if (width() <= 0 || height() <= 0) return;

    auto* rng = QRandomGenerator::global();
    const int target = qBound(40, (width() * height()) / 14000, 140);
    m_stars.reserve(target);
    for (int i = 0; i < target; ++i) {
        Star s;
        s.x             = rng->generateDouble();
        s.y             = rng->generateDouble();
        s.baseAlpha     = 0.18 + rng->generateDouble() * 0.50;
        s.twinklePhase  = rng->generateDouble() * 6.2831;
        s.twinkleSpeed  = 0.4  + rng->generateDouble() * 1.4;
        s.radius        = 0.6  + rng->generateDouble() * 1.1;
        m_stars.append(s);
    }
}

void ParticleBackground::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0) return;
    const qreal minWH = qMin(w, h);

    // ---- 1. Deep obsidian backdrop ----
    QLinearGradient backdrop(0, 0, 0, h);
    backdrop.setColorAt(0.00, QColor(6,  9,  14));
    backdrop.setColorAt(0.55, QColor(4,  7,  12));
    backdrop.setColorAt(1.00, QColor(2,  5,   9));
    p.fillRect(rect(), backdrop);

    // ---- 2. Aurora blobs (additive-ish: drawn with alpha over backdrop) ----
    p.setPen(Qt::NoPen);
    p.setCompositionMode(QPainter::CompositionMode_Plus); // additive glow
    for (const Blob& b : m_blobs) {
        const qreal cx = (0.5 + b.ax * std::cos(b.wx * m_time + b.px) * 0.5) * w;
        const qreal cy = (0.5 + b.ay * std::sin(b.wy * m_time + b.py) * 0.5) * h;
        const qreal r  = b.radius * minWH;

        QRadialGradient rg(QPointF(cx, cy), r);
        QColor inner = b.color;
        inner.setAlpha(int(b.alpha * 255 * 0.75));
        QColor outer = b.color;
        outer.setAlpha(0);
        rg.setColorAt(0.00, inner);
        rg.setColorAt(0.55, QColor(inner.red(), inner.green(), inner.blue(),
                                   int(b.alpha * 255 * 0.20)));
        rg.setColorAt(1.00, outer);
        p.setBrush(rg);
        p.drawEllipse(QPointF(cx, cy), r, r);
    }
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // ---- 3. Subtle starfield ----
    for (const Star& s : m_stars) {
        const qreal alpha =
            s.baseAlpha * (0.55 + 0.45 * std::sin(m_time * s.twinkleSpeed + s.twinklePhase));
        QColor c(220, 232, 248, int(qBound(0.0, alpha, 1.0) * 255));
        p.setBrush(c);
        p.drawEllipse(QPointF(s.x * w, s.y * h), s.radius, s.radius);
    }

    // ---- 4. Vignette: dark corners, slightly brighter center ----
    {
        QRadialGradient vg(QPointF(w * 0.5, h * 0.5),
                           std::sqrt(w * w + h * h) * 0.6);
        vg.setColorAt(0.00, QColor(0, 0, 0, 0));
        vg.setColorAt(0.65, QColor(0, 0, 0, 60));
        vg.setColorAt(1.00, QColor(0, 0, 0, 200));
        p.setBrush(vg);
        p.setPen(Qt::NoPen);
        p.drawRect(rect());
    }
}
