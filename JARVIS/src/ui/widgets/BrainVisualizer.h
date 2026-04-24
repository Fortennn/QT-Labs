#ifndef BRAIN_VISUALIZER_H
#define BRAIN_VISUALIZER_H

#include <QWidget>
#include <QTimer>
#include <QVector3D>
#include <QVector>
#include <QPaintEvent>
#include <QResizeEvent>

class BrainVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit BrainVisualizer(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateAnimation();

private:
    struct Node {
        QVector3D pos;
        QVector3D velocity;
    };

    QVector<Node> nodes;
    QTimer* timer;
    float rotationAngle = 0.0f;
    
    void initNodes();
};

#endif // BRAIN_VISUALIZER_H
