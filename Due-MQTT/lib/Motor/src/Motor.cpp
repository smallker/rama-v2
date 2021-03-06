#include <Arduino.h>
#include "Motor.h"

/*
    Mode tanpa encoder
    a_pin   -> Motor A pin
    b_pin   -> Motor B pin
    pwm_pin -> Motor PWM pin
*/
Motor::Motor(byte a_pin, byte b_pin, byte pwm_pin)
{
    this->a_pin = a_pin;
    this->b_pin = b_pin;
    this->pwm_pin = pwm_pin;

    pinMode(this->a_pin, OUTPUT);
    pinMode(this->b_pin, OUTPUT);
    pinMode(this->pwm_pin, OUTPUT);
    // setPwmFrequency();
}
/*
    Mode dengan encoder
    a_pin   -> Motor A pin
    b_pin   -> Motor B pin
    pwm_pin -> Motor PWM pin
    en_a    -> Encoder channel A
    en_b    -> Encoder channel B
*/
Motor::Motor(byte a_pin, byte b_pin, byte pwm_pin, byte en_a, byte en_b)
{
    this->a_pin = a_pin;
    this->b_pin = b_pin;
    this->pwm_pin = pwm_pin;
    this->en_a = en_a;
    this->en_b = en_b;
    pinMode(this->a_pin, OUTPUT);
    pinMode(this->b_pin, OUTPUT);
    pinMode(this->pwm_pin, OUTPUT);
    pinMode(this->en_a, INPUT_PULLUP);
    pinMode(this->en_b, INPUT_PULLUP);
    digitalWrite(this->a_pin, LOW);
    digitalWrite(this->b_pin, LOW);
    digitalWrite(this->pwm_pin, LOW);
    // setPwmFrequency();
}

/*
    Mode dengan encoder dengan setting manual PPR
    PPR default 135 (PG-36)
    a_pin   -> Motor A pin
    b_pin   -> Motor B pin
    pwm_pin -> Motor PWM pin
    en_a    -> Encoder channel A
    en_b    -> Encoder channel B
*/
Motor::Motor(byte a_pin, byte b_pin, byte pwm_pin, byte en_a, byte en_b, int ppr)
{
    this->a_pin = a_pin;
    this->b_pin = b_pin;
    this->pwm_pin = pwm_pin;
    this->en_a = en_a;
    this->en_b = en_b;
    this->ppr = ppr;
    pinMode(this->a_pin, OUTPUT);
    pinMode(this->b_pin, OUTPUT);
    pinMode(this->pwm_pin, OUTPUT);
    pinMode(this->en_a, INPUT_PULLUP);
    pinMode(this->en_b, INPUT_PULLUP);
    digitalWrite(this->a_pin, LOW);
    digitalWrite(this->b_pin, LOW);
    digitalWrite(this->pwm_pin, LOW);
    // setPwmFrequency();
}
/*
    Kp, Ki, Kd, Windup
    
    Kp
    + Cepat mencapai setpoint
    - Menimbulkan osilasi
    
    Ki
    + Koreksi pada interval tertentu
    - Menimbulkan windup
    
    Kd
    + Menahan kondisi stabil, meredam osilasi
    - Akan stuck pada kondisi sebelum setpoint

    Rumus PID = (kp * error * 0.1) + (ki * integral error* 0.01) + (kd * (error - last Error) * 0.1)
*/
void Motor::pid(float kp, float ki, float kd, float windup)
{
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
    this->windup = windup;
    this->pidEnable = true;
}
/*
    Atur batas PWM PID yang dihasilkan
    Range yang diperbolehkan antara 0-255
    default 0-150
*/
void Motor::setPidThreshold(int min_pwm, int max_pwm)
{
    this->min_pwm = min_pwm;
    this->max_pwm = max_pwm;
}

/*
    Mengatur kecepatan motor, apabila PID dimatikan maka akan menggunakan
    nilai dari pwm yg dimasukkan. Apabila PID enable maka nilai yang
    dimasukkan adalah nilai setpoint RPM. Nilai minus untuk CCW
    dan nilai plus untuk CW
*/
void Motor::speed(int target)
{
    if (pidEnable == true)
    {
        err = abs(target) - rpm;
        abs(mIntegral < windup) ? mIntegral += err : mIntegral = 0;
        pwmPid = (kp * err * 0.1) + (ki * mIntegral * 0.01) + (kd * (err - lastErr) * 0.1);
        lastErr = err;
        pwmPid = abs(target) + pwmPid;
        if (pwmPid < min_pwm)
        {
            pwmPid = min_pwm;
        }
        else if (pwmPid > max_pwm)
        {
            pwmPid = max_pwm;
        }
        Serial.println("kp : " + (String)kp + " ki : " + (String)ki + " kd : " + (String)kd + " pwm :" + (String)pwmPid);
        target > 0 ? forward(pwmPid) : reverse(pwmPid);
    }
    else
    {
        if (target > 255)
            target = 255;
        else if (target < -255)
            target = -255;
        target > 0 ? forward(target) : reverse(target);
    }
}

/*
    Motor bergerak clock wise
*/
void Motor::forward(int pwm)
{
    if (abs(pwm) < threshold)
        pwm = 0;
    digitalWrite(this->a_pin, HIGH);
    digitalWrite(this->b_pin, LOW);
    // analogWrite(this->pwm_pin, pwm);
    analogWrite(this->pwm_pin, pwm);
}

/*
    Motor bergerak counter clock wise
*/
void Motor::reverse(int pwm)
{
    if (abs(pwm) < threshold)
        pwm = 0;
    digitalWrite(this->a_pin, LOW);
    digitalWrite(this->b_pin, HIGH);
    // analogWrite(this->pwm_pin, abs(pwm));
    analogWrite(this->pwm_pin, abs(pwm));
}

#if defined(EMS)
/*
    Khusus driver EMS terdapat fungsi untuk mengerem
    motor dengan membuat pin A dan pin B menjadi HIGH
*/
void Motor::brake()
{
    digitalWrite(this->a_pin, HIGH);
    digitalWrite(this->b_pin, HIGH);
}
#else

/*
    Motor berhenti, bisa diatur untuk driver EMS
    dengan mendefinisikan #define EMS pada Motor.h
*/
void Motor::brake()
{
    digitalWrite(this->a_pin, LOW);
    digitalWrite(this->b_pin, LOW);
}
#endif

/*
    Fungsi berikut adalah untuk merecord hasil pembacaan encoder motor.
    encoder_tick adalah variabel yang akan direset pada interval tertentu
    untuk mendapatkan kecepatan motor, sedangkan encoder_tick_acc
    adalah variabel yang tidak akan direset selama sistem berjalan
*/
void Motor::isrHandler()
{
    digitalRead(b_pin) == HIGH ? encoder_tick++ : encoder_tick--;
    digitalRead(b_pin) == HIGH ? encoder_tick_acc++ : encoder_tick_acc--;
}

/*
    Masukkan waktu sampling (ms) sama persis dengan waktu timer interrupt.
    Misal sampling rpm adalah 10 ms:

    int sampling_time_ms = 10;
    Timer1.initialize(1000 * sampling_time_ms);
    maka
    m.calculateRpm(sampling_time_ms);
    
*/
void Motor::calculateRpm(int sampling_time_ms)
{
#if (SAM3XA_SERIES) || (SAM3N_SERIES) || (SAM3S_SERIES)
    sampling_time_ms = sampling_time_ms / 2;
#endif
    rpm = abs((encoder_tick / ppr) * (60000 / sampling_time_ms));
    encoder_tick = 0;
}

void Motor::setPwmThreshold(int threshold)
{
    this->threshold = threshold;
}

void Motor::setPwmFrequency()
{
#if (SAM3XA_SERIES) || (SAM3N_SERIES) || (SAM3S_SERIES)
    uint32_t pwmPin = pwm_pin;
    uint32_t maxDutyCount = 2;
    uint32_t clkAFreq = 42000000ul;
    uint32_t pwmFreq = 42000000ul;
    pmc_enable_periph_clk(PWM_INTERFACE_ID);
    PWMC_ConfigureClocks(clkAFreq, 0, VARIANT_MCK);
    PIO_Configure(
        g_APinDescription[pwmPin].pPort,
        g_APinDescription[pwmPin].ulPinType,
        g_APinDescription[pwmPin].ulPin,
        g_APinDescription[pwmPin].ulPinConfiguration);
    uint32_t channel = g_APinDescription[pwmPin].ulPWMChannel;
    PWMC_ConfigureChannel(PWM_INTERFACE, channel, pwmFreq, 0, 0);
    PWMC_SetPeriod(PWM_INTERFACE, channel, maxDutyCount);
    PWMC_EnableChannel(PWM_INTERFACE, channel);
    PWMC_SetDutyCycle(PWM_INTERFACE, channel, 1);
    pmc_mck_set_prescaler(2);
#else
#endif
}