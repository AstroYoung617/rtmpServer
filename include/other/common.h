
#pragma once

#include <chrono>
#include <regex>


namespace common {

  using std::string;

  class Time {
  public:
    static int64_t currentTime() {
      using namespace std::chrono;
      auto time_now = system_clock::now();
      auto durationIn = duration_cast<milliseconds>(time_now.time_since_epoch());
      return durationIn.count();
    };

    static int64_t microTime() {
      using namespace std::chrono;
      auto time_now = system_clock::now();
      auto durationIn = duration_cast<microseconds>(time_now.time_since_epoch());
      return durationIn.count();
    };

    static string parseTime(int64_t timestamp) {
      auto mTime = std::chrono::milliseconds(timestamp + (int64_t)8 * 60 * 60 * 1000);
      auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
      auto tt = std::chrono::system_clock::to_time_t(tp);
      std::tm* now = std::gmtime(&tt);
      string s = std::to_string(now->tm_year + 1900) + std::to_string(now->tm_mon + 1) + std::to_string(now->tm_mday) + " " +
        std::to_string(now->tm_hour) + ":" + std::to_string(now->tm_min) + ":" + std::to_string(now->tm_sec);
      return s;
    };


    //timestamp -> yyyy-MM-dd HH:mm:ss
    static string timestamp2string(int64_t timestamp) {
      auto mTime = std::chrono::milliseconds(timestamp + (int64_t)8 * 60 * 60 * 1000);
      auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
      auto tt = std::chrono::system_clock::to_time_t(tp);
      std::tm* now = std::gmtime(&tt);
      char timeBuf[64] = { '\0' };
      strftime(timeBuf, 64, "%Y-%m-%d %H:%M:%S", now);
      string timeStr = timeBuf;
      return timeStr;
    }

    //timestamp -> yyyy-MM-dd
    static string timestamp2date1(int64_t timestamp) {
      auto mTime = std::chrono::milliseconds(timestamp + (int64_t)8 * 60 * 60 * 1000);
      auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
      auto tt = std::chrono::system_clock::to_time_t(tp);
      std::tm* now = std::gmtime(&tt);
      char timeBuf[64] = { '\0' };
      strftime(timeBuf, 64, "%Y-%m-%d", now);
      string timeStr = timeBuf;
      return timeStr;
    }

    //timestamp -> yyyyMMdd
    static string timestamp2date2(int64_t timestamp) {
      auto mTime = std::chrono::milliseconds(timestamp + (int64_t)8 * 60 * 60 * 1000);
      auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
      auto tt = std::chrono::system_clock::to_time_t(tp);
      std::tm* now = std::gmtime(&tt);
      char timeBuf[64] = { '\0' };
      strftime(timeBuf, 64, "%Y%m%d", now);
      string timeStr = timeBuf;
      return timeStr;
    }

    //yyyy-MM-dd HH:mm:ss -> second
    static int64_t timeStr2second(string timeStr)
    {
      char* cha = (char*)timeStr.data();         // 将string转换成char*。
      tm tm_;                                    // 定义tm结构体。
      int year, month, day, hour, minute, second;// 定义时间的各个int临时变量。
      sscanf(cha, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);// 将string存储的日期时间，转换为int临时变量。
      tm_.tm_year = year - 1900;                 // 年，由于tm结构体存储的是从1900年开始的时间，所以tm_year为int临时变量减去1900。
      tm_.tm_mon = month - 1;                    // 月，由于tm结构体的月份存储范围为0-11，所以tm_mon为int临时变量减去1。
      tm_.tm_mday = day;                         // 日。
      tm_.tm_hour = hour;                        // 时。
      tm_.tm_min = minute;                       // 分。
      tm_.tm_sec = second;                       // 秒。
      tm_.tm_isdst = 0;                          // 非夏令时。
      time_t t_ = mktime(&tm_);                  // 将tm结构体转换成time_t格式。
      int64_t timestamp = t_;
      return timestamp;
    }
  };

  class String {
  public:
    static string toLower(const string& target) {
      string out{};
      for (auto c : target) {
        out += ::tolower(c);
      }
      return out;
    }

    static string toUpper(const string& target) {
      string out{};
      for (auto c : target) {
        out += ::toupper(c);
      }
      return out;
    }

    static string trim(string& s) {
      string rst{ s };
      if (rst.empty()) {
        return rst;
      }
      rst.erase(0, rst.find_first_not_of(" "));
      rst.erase(s.find_last_not_of(" ") + 1);
      return rst;
    }

    static std::vector<string> split(const string& target, const string& sp) {
      std::vector<string> rst{};
      if (target.size() == 0) {
        return rst;
      }

      const auto spLen = sp.length();
      string::size_type pos = 0;
      auto f = target.find(sp, pos);

      while (f != string::npos) {
        auto r = target.substr(pos, f - pos);
        rst.emplace_back(r);
        pos = f + spLen;
        f = target.find(sp, pos);
      }
      rst.emplace_back(target.substr(pos, target.length()));
      return rst;
    }

    static string removeBlanks(const string& target) {
      static std::regex blankRe{ R"(\s+)" };
      return std::regex_replace(target, blankRe, "");
    }

    static string removeLastEmptyLines(const string& target) {
      size_t len = target.length();
      size_t i = len - 1;
      for (; i > 0; i--) {
        if (target[i] == '\n') {
          continue;
        }
        else if (target[i] == '\r') {
          continue;
        }
        else {
          break;
        }
      }
      return target.substr(0, i + 1);
    }
  };


  class ByteArray {
  public:
    template<typename T>
    static void writeData(uint8_t* buf, T num) {
      int len(sizeof(T));
      for (size_t i = 0; i < len; ++i) {
        buf[i] = (uint8_t)(num >> (i * 8));
      }
    }

    template<typename T>
    static void readData(uint8_t* buf, T& num) {
      uint8_t len(sizeof(T));
      static uint8_t byMask(0xFF);
      num = 0;

      for (size_t i = 0; i < len; ++i) {
        num <<= 8;
        num |= (T)(buf[len - 1 - i] & byMask);
      }
    }
  };


}  // namespace common