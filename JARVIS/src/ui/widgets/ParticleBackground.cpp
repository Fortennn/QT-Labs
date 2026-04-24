#include "ParticleBackground.h"
#include <QPainter>
#include <QRandomGenerator>

ParticleBackground::ParticleBackground(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    
    // Створюємо зірки
    for (int i = 0; i < 200; ++i) {
        Star s;
        s.x = (float)QRandomGenerator::global()->generateDouble();
        s.y = (float)QRandomGenerator::global()->generateDouble();
        s.z = (float)QRandomGenerator::global()->generateDouble() * 2.0f;
        s.speed = (float)QRandomGenerator::global()->generateDouble() * 0.0005f + 0.0001f;
        s.opacity = QRandomGenerator::global()->bounded(50, 200);
        stars.append(s);
    }
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ParticleBackground::animate);
    timer->start(16); // ~60 FPS
}

void ParticleBackground::animate() {
    for (auto &s : stars) {
        s.y += s.speed;
        if (s.y > 1.0f) {
            s.y = 0.0f;
            s.x = (float)QRandomGenerator::global()->generateDouble();
        }
    }
    update();
}

void ParticleBackground::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Глибокий градієнт космосу
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0, QColor(2, 5, 15));
    bg.setColorAt(1, QColor(5, 10, 30));
    p.fillRect(rect(), bg);
    
    for (const auto &s : stars) {
        float r = (s.z > 1.5f) ? 1.5f : 0.8f;
        p.setPen(Qt::NoPen);
        
        // Сяйво навколо зірки
        QRadialGradient glow(QPointF(s.x * width(), s.y * height()), r * 2);
        glow.setColorAt(0, QColor(255, 255, 255, s.opacity));
        glow.setColorAt(1, QColor(255, 255, 255, 0));
        
        p.setBrush(glow);
        p.drawEllipse(QPointF(s.x * width(), s.y * height()), r * 2, r * 2);
        
        p.setBrush(QColor(255, 255, 255, s.opacity));
        p.drawEllipse(QPointF(s.x * width(), s.y * height()), r, r);
    }
}
