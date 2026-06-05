#include "VideoPlayerWidget.h"
#include "util/TimeUtil.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVideoWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QAudioOutput>

VideoPlayerWidget::VideoPlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    m_player = new QMediaPlayer(this);
    auto *audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(audioOutput);

    m_videoWidget = new QVideoWidget(this);
    m_player->setVideoOutput(m_videoWidget);

    m_playButton = new QPushButton(tr("Play"), this);
    m_positionSlider = new QSlider(Qt::Horizontal, this);
    m_timeLabel = new QLabel("00:00:00.000 / 00:00:00.000", this);

    auto *controls = new QHBoxLayout;
    controls->addWidget(m_playButton);
    controls->addWidget(m_positionSlider, 1);
    controls->addWidget(m_timeLabel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_videoWidget, 1);
    layout->addLayout(controls);

    connect(m_playButton, &QPushButton::clicked, this, &VideoPlayerWidget::togglePlayPause);
    connect(m_positionSlider, &QSlider::sliderPressed, this, [this]() { m_sliderDragging = true; });
    connect(m_positionSlider, &QSlider::sliderReleased, this, [this]() {
        m_sliderDragging = false;
        onSliderMoved(m_positionSlider->value());
    });
    connect(m_positionSlider, &QSlider::sliderMoved, this, &VideoPlayerWidget::onSliderMoved);

    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPlayerWidget::onPlayerPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPlayerWidget::onPlayerDurationChanged);
}

void VideoPlayerWidget::loadVideo(const QString &filePath)
{
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_player->pause();
}

void VideoPlayerWidget::seekTo(qint64 positionMs)
{
    m_player->setPosition(positionMs);
}

qint64 VideoPlayerWidget::duration() const
{
    return m_player->duration();
}

qint64 VideoPlayerWidget::position() const
{
    return m_player->position();
}

void VideoPlayerWidget::play()
{
    m_player->play();
    m_playButton->setText(tr("Pause"));
}

void VideoPlayerWidget::pause()
{
    m_player->pause();
    m_playButton->setText(tr("Play"));
}

void VideoPlayerWidget::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        pause();
    } else {
        play();
    }
}

void VideoPlayerWidget::onPlayerPositionChanged(qint64 pos)
{
    if (!m_sliderDragging) {
        m_positionSlider->setValue(static_cast<int>(pos));
    }
    m_timeLabel->setText(QString("%1 / %2")
        .arg(TimeUtil::msToDisplay(pos))
        .arg(TimeUtil::msToDisplay(m_player->duration())));
    emit positionChanged(pos);
}

void VideoPlayerWidget::onPlayerDurationChanged(qint64 dur)
{
    m_positionSlider->setRange(0, static_cast<int>(dur));
    emit durationChanged(dur);
}

void VideoPlayerWidget::onSliderMoved(int value)
{
    m_player->setPosition(value);
}
