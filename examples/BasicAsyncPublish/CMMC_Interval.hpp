#ifndef CMMC_NETPIE_INTERVAL_H
#define CMMC_NETPIE_INTERVAL_H
#include <Arduino.h>
#include <functional>

class CMMC_Interval
{
  private:
    unsigned long _prev;
    unsigned long _now;
    unsigned long _threshold_ms;

  public:
    typedef std::function<void(void)> void_callback_t;

    CMMC_Interval() {
      _prev = millis();
      _now  = millis();
    };

    ~CMMC_Interval() {
    };

    void every_ms(unsigned long ms, void_callback_t cb) {
      _threshold_ms = ms;
      _now  = millis();
      unsigned long diff = _now - _prev;
      if (diff >= _threshold_ms) {
        _prev = millis();
        cb();
      }
    }
};


#endif

