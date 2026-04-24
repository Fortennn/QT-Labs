#ifndef PARTICLE_BACKGROUND_H
#define PARTICLE_BACKGROUND_H

#include <QWidget>
#include <QTimer>
#include <QVector>

class ParticleBackground : public QWidget {
    Q_OBJECT
public:
    explicit ParticleBackground(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void animate();

private:
    struct Star {
        float x, y, z, speed;
        int opacity;
    };
    QVector<Star> stars;
    QTimer* timer;
};

#endif // PARTICLE_BACKGROUND_H
