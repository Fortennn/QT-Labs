#ifndef PARTICLE_BACKGROUND_H
#define PARTICLE_BACKGROUND_H

#include <QColor>
#include <QPointF>
#include <QTimer>
#include <QVector>
#include <QWidget>

// Despite the name (kept for ABI / include-path compatibility with the rest of
// the project), this is no longer a starfield. It's a soft animated "aurora"
// background: a few large, low-alpha radial-gradient blobs slowly drifting
// across a deep obsidian backdrop, plus a subtle starfield and vignette on top.
class ParticleBackground : public QWidget {
    Q_OBJECT

public:
    explicit ParticleBackground(QWidget* parent = nullptr);

    // Cosmetic accent color used by the aurora blobs. Defaults to JARVIS-blue.
    void setAccentColor(const QColor& accent);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void tick();

private:
    struct Blob {
        QColor   color;
        qreal   ax, ay;     // amplitude
        qreal   px, py;     // phase
        qreal   wx, wy;     // angular speed
        qreal   radius;     // in fraction of min(w,h)
        qreal   alpha;      // 0..1
    };

    struct Star {
        qreal x, y;          // 0..1 normalized
        qreal baseAlpha;     // 0..1
        qreal twinklePhase;
        qreal twinkleSpeed;
        qreal radius;
    };

    void rebuildStars();

    QVector<Blob>  m_blobs;
    QVector<Star>  m_stars;
    QTimer*        m_timer  = nullptr;
    qreal          m_time   = 0.0;          // seconds
    QColor         m_accent = QColor(47, 129, 247);
};

#endif // PARTICLE_BACKGROUND_H
