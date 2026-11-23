#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>
#include <QRandomGenerator>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <functional>

class SnakeBoard : public QWidget {
public:
    explicit SnakeBoard(QWidget* parent = nullptr) : QWidget(parent), timer_(new QTimer(this)) {
        setMinimumSize(600, 430);
        setFocusPolicy(Qt::StrongFocus);
        connect(timer_, &QTimer::timeout, this, [this]() { advanceGame(); });
        restart();
    }

    void setScoreCallback(std::function<void(int)> callback) { scoreCallback_ = std::move(callback); }
    void setStatusCallback(std::function<void(const QString&)> callback) { statusCallback_ = std::move(callback); }

    void restart() {
        snake_ = {QPoint(9, 9), QPoint(8, 9), QPoint(7, 9)};
        direction_ = QPoint(1, 0);
        nextDirection_ = direction_;
        score_ = 0;
        gameOver_ = false;
        spawnFood();
        timer_->start(135);
        if (scoreCallback_) scoreCallback_(score_);
        if (statusCallback_) statusCallback_(QStringLiteral("با کلیدهای جهت‌دار یا WASD حرکت کن"));
        setFocus();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#0b1220"));
        const int cell = qMin(width() / columns_, height() / rows_);
        const int boardW = cell * columns_;
        const int boardH = cell * rows_;
        const int left = (width() - boardW) / 2;
        const int top = (height() - boardH) / 2;
        const QRect area(left, top, boardW, boardH);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#111d30"));
        p.drawRoundedRect(area, 15, 15);
        p.setPen(QPen(QColor("#1a2a40"), 1));
        for (int x = 1; x < columns_; ++x) p.drawLine(left + x * cell, top, left + x * cell, top + boardH);
        for (int y = 1; y < rows_; ++y) p.drawLine(left, top + y * cell, left + boardW, top + y * cell);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ff647c"));
        p.drawEllipse(QRect(left + food_.x() * cell + 4, top + food_.y() * cell + 4, cell - 8, cell - 8));
        for (int i = snake_.size() - 1; i >= 0; --i) {
            const QPoint part = snake_.at(i);
            const QRect segment(left + part.x() * cell + 2, top + part.y() * cell + 2, cell - 4, cell - 4);
            p.setBrush(i == 0 ? QColor("#b7ff65") : QColor("#39c879"));
            p.drawRoundedRect(segment, 6, 6);
        }
        if (gameOver_) {
            p.fillRect(area, QColor(5, 10, 18, 185));
            p.setPen(Qt::white);
            QFont f = p.font();
            f.setPointSize(24);
            f.setBold(true);
            p.setFont(f);
            p.drawText(area, Qt::AlignCenter, QStringLiteral("GAME OVER\nSpace برای شروع مجدد"));
        }
    }

    void keyPressEvent(QKeyEvent* e) override {
        if (gameOver_ && e->key() == Qt::Key_Space) {
            restart();
            return;
        }
        QPoint candidate = nextDirection_;
        switch (e->key()) {
        case Qt::Key_Up: case Qt::Key_W: candidate = QPoint(0, -1); break;
        case Qt::Key_Down: case Qt::Key_S: candidate = QPoint(0, 1); break;
        case Qt::Key_Left: case Qt::Key_A: candidate = QPoint(-1, 0); break;
        case Qt::Key_Right: case Qt::Key_D: candidate = QPoint(1, 0); break;
        default: QWidget::keyPressEvent(e); return;
        }
        if (candidate + direction_ != QPoint(0, 0)) nextDirection_ = candidate;
        e->accept();
    }

private:
    void advanceGame() {
        if (gameOver_) return;
        direction_ = nextDirection_;
        const QPoint head = snake_.front() + direction_;
        if (head.x() < 0 || head.x() >= columns_ || head.y() < 0 || head.y() >= rows_) {
            finishGame();
            return;
        }
        for (int i = 0; i < snake_.size() - 1; ++i) {
            if (snake_.at(i) == head) {
                finishGame();
                return;
            }
        }
        snake_.prepend(head);
        if (head == food_) {
            score_ += 10;
            spawnFood();
            timer_->setInterval(qMax(55, 135 - (score_ / 30) * 8));
            if (scoreCallback_) scoreCallback_(score_);
        } else {
            snake_.removeLast();
        }
        update();
    }

    void finishGame() {
        gameOver_ = true;
        timer_->stop();
        if (statusCallback_) statusCallback_(QStringLiteral("بازی تمام شد؛ برای شروع دوباره Space را بزن"));
        update();
    }

    void spawnFood() {
        do {
            food_ = QPoint(QRandomGenerator::global()->bounded(columns_),
                           QRandomGenerator::global()->bounded(rows_));
        } while (snake_.contains(food_));
    }

    static constexpr int columns_ = 24;
    static constexpr int rows_ = 18;
    QVector<QPoint> snake_;
    QPoint direction_{1, 0};
    QPoint nextDirection_{1, 0};
    QPoint food_{12, 9};
    QTimer* timer_;
    int score_ = 0;
    bool gameOver_ = false;
    std::function<void(int)> scoreCallback_;
    std::function<void(const QString&)> statusCallback_;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    QWidget window;
    window.setWindowTitle(QStringLiteral("Snake Game | Qt C++"));
    window.resize(850, 650);
    window.setMinimumSize(680, 560);
    window.setStyleSheet(R"(
        QWidget { background: #0b1220; color: #edf4f0; font-family: "Segoe UI"; }
        QLabel#title { color: #b7ff65; font-size: 25px; font-weight: 800; }
        QLabel#subtitle { color: #8294a8; font-size: 11px; }
        QLabel#score { color: #b7ff65; background: #142438; border: 1px solid #263b52; border-radius: 12px; padding: 9px 18px; font-size: 16px; font-weight: 700; }
        QLabel#status { color: #9aadc0; font-size: 11px; }
        QPushButton { background: #b7ff65; color: #102018; border: none; border-radius: 10px; padding: 11px 20px; font-weight: 800; }
        QPushButton:hover { background: #d0ff9c; }
    )");
    auto* root = new QVBoxLayout(&window);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);
    auto* header = new QHBoxLayout;
    auto* titleBox = new QVBoxLayout;
    auto* title = new QLabel(QStringLiteral("SNAKE ARCADE"));
    title->setObjectName("title");
    auto* subtitle = new QLabel(QStringLiteral("Qt Widgets  •  C++  •  Dark Edition"));
    subtitle->setObjectName("subtitle");
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    auto* score = new QLabel(QStringLiteral("امتیاز: ۰"));
    score->setObjectName("score");
    auto* restart = new QPushButton(QStringLiteral("شروع مجدد"));
    header->addLayout(titleBox);
    header->addStretch();
    header->addWidget(score);
    header->addWidget(restart);
    auto* board = new SnakeBoard;
    auto* status = new QLabel(QStringLiteral("با کلیدهای جهت‌دار یا WASD حرکت کن"));
    status->setObjectName("status");
    status->setAlignment(Qt::AlignCenter);
    board->setScoreCallback([score](int value) { score->setText(QStringLiteral("امتیاز: %1").arg(value)); });
    board->setStatusCallback([status](const QString& value) { status->setText(value); });
    QObject::connect(restart, &QPushButton::clicked, board, [board]() { board->restart(); });
    root->addLayout(header);
    root->addWidget(board, 1);
    root->addWidget(status);
    window.show();
    board->setFocus();
    return app.exec();
}
