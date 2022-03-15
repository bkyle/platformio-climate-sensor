#pragma once

class Interval {
  private:
    long _interval;
    long _timeout;
  public:
    Interval(long interval);
    bool check();
};