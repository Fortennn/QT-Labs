#include "BrainVisualizer.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include <QPaintEvent>
#include <QResizeEvent>

BrainVisualizer::BrainVisualizer(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    initNodes();
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BrainVisualizer::updateAnimation);
    timer->start(33); // ~30 FPS
}

void BrainVisualizer::initNodes() {
    nodes.clear();
    const int count = 60; // Оптимальна кількість для чіткості
    const float PI = 3.14159265358979323846f;
    for (int i = 0; i < count; ++i) {
        float theta = QRandomGenerator::global()->generateDouble() * PI * 2;
        float phi = qAcos(2.0 * QRandomGenerator::global()->generateDouble() - 1.0);
        float r = 100.0;
        
        Node n;
        n.pos = QVector3D(r * qSin(phi) * qCos(theta),
                          r * qSin(phi) * qSin(theta),
                          r * qCos(phi));
        
        n.velocity = QVector3D(
            (QRandomGenerator::global()->generateDouble() - 0.5) * 0.5,
            (QRandomGenerator::global()->generateDouble() - 0.5) * 0.5,
            (QRandomGenerator::global()->generateDouble() - 0.5) * 0.5
        );
        nodes.append(n);
    }
}

void BrainVisualizer::updateAnimation() {
    rotationAngle += 0.01f;
    
    for (auto &node : nodes) {
        node.pos += node.velocity;
        
        // Keep in sphere
        if (node.pos.length() > 120.0f) {
            node.velocity = -node.velocity;
        }
    }
    update();
}

void BrainVisualizer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void BrainVisualizer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    painter.translate(w / 2, h / 2);
    
    float cosA = qCos(rotationAngle);
    float sinA = qSin(rotationAngle);
    
    auto project = [&](QVector3D p) {
        // Rotate around Y
        float x = p.x() * cosA - p.z() * sinA;
        float z = p.x() * sinA + p.z() * cosA;
        float y = p.y();
        
        // Simple perspective
        float factor = 300.0f / (300.0f + z);
        return QPointF(x * factor, y * factor);
    };

    QVector<QPointF> projected;
    for (const auto &n : nodes) {
        projected.append(project(n.pos));
    }
    
    // Draw connections with glowing effect
    float pulse = (qSin(rotationAngle * 2.0f) + 1.0f) / 2.0f; // Пульсація 0..1
    
    for (int i = 0; i < nodes.size(); ++i) {
        for (int j = i + 1; j < nodes.size(); ++j) {
            float dist = (nodes[i].pos - nodes[j].pos).length();
            if (dist < 70.0f) {
                float alpha = (1.0f - (dist / 70.0f)) * (0.3f + 0.7f * pulse);
                QColor lineColor(0, 200, 255, alpha * 180);
                
                QPen pen(lineColor);
                pen.setWidthF(1.0 + pulse * 0.5);
                painter.setPen(pen);
                painter.drawLine(projected[i], projected[j]);
            }
        }
    }
    
    // Draw nodes
    for (int i = 0; i < projected.size(); ++i) {
        float z = nodes[i].pos.x() * sinA + nodes[i].pos.z() * cosA;
        float size = 4.0f * (300.0f / (300.0f + z));
        
        painter.setPen(Qt::NoPen);
        QRadialGradient grad(projected[i], size);
        grad.setColorAt(0, QColor(0, 255, 255, 255));
        grad.setColorAt(1, QColor(0, 150, 255, 0));
        painter.setBrush(grad);
        painter.drawEllipse(projected[i], size, size);
    }
}
