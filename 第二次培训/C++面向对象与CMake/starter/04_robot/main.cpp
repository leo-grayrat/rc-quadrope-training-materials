#include <iomanip>
#include <iostream>

class Motor {
public:
    virtual void enable() = 0;
    virtual void setPosition(double position) = 0;
    virtual double getPosition() const = 0;
    virtual ~Motor() = default;
};

class DMMotor : public Motor {
public:
    explicit DMMotor(int id)
        : id_(id), position_(0.0)
    {
    }

    void enable() override
    {
        std::cout << "DM Motor " << id_ << " enabled." << std::endl;
    }

    void setPosition(double position) override
    {
        position_ = position;
        std::cout << "DM Motor " << id_ << " -> target = "
                  << std::fixed << std::setprecision(2)
                  << position_ << " rad" << std::endl;
    }

    double getPosition() const override
    {
        return position_;
    }

private:
    int id_;
    double position_;
};

class UnitreeMotor : public Motor {
public:
    explicit UnitreeMotor(int id)
        : id_(id), position_(0.0)
    {
    }

    void enable() override
    {
        std::cout << "Unitree Motor " << id_ << " enabled." << std::endl;
    }

    void setPosition(double position) override
    {
        position_ = position;
        std::cout << "Unitree Motor " << id_ << " -> target = "
                  << std::fixed << std::setprecision(2)
                  << position_ << " rad" << std::endl;
    }

    double getPosition() const override
    {
        return position_;
    }

private:
    int id_;
    double position_;
};

class Robot {
public:
    Robot(Motor& left_motor, Motor& right_motor)
        : left_motor_(left_motor),
          right_motor_(right_motor)
    {
    }

    // TODO: initialize()
    // 输出 "Robot initialization..."，然后依次 enable 左右电机。

    // TODO: move(double position)
    // 输出目标位置，然后把同一个目标发送给左右电机。

    // TODO: printStatus() const
    // 输出左右电机当前位置。

private:
    Motor& left_motor_;
    Motor& right_motor_;
};

int main()
{
    DMMotor left_motor(1);
    UnitreeMotor right_motor(2);
    Robot robot(left_motor, right_motor);

    robot.initialize();
    robot.move(1.50);
    robot.printStatus();

    return 0;
}
