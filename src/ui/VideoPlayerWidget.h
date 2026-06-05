#pragma once

#include <QWidget>
#include <QMediaPlayer>

class QVideoWidget;
class QSlider;
class QLabel;
class QPushButton;

class VideoPlayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPlayerWidget(QWidget *parent = nullptr);

    void loadVideo(const QString &filePath);
    void seekTo(qint64 positionMs);
    qint64 duration() const;
    qint64 position() const;

signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);

public slots:
    void play();
    void pause();
    void togglePlayPause();

private slots:
    void onPlayerPositionChanged(qint64 pos);
    void onPlayerDurationChanged(qint64 dur);
    void onSliderMoved(int value);

private:
    QMediaPlayer  *m_player;
    QVideoWidget  *m_videoWidget;
    QSlider       *m_positionSlider;
    QLabel        *m_timeLabel;
    QPushButton   *m_playButton;
    bool           m_sliderDragging = false;
};
