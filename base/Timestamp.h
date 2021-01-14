//
// Created by Kukai on 2020/12/14.
//

#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include <stdint.h>
#include <string>

namespace Kukai{

    class Timestamp {
    public:

        Timestamp();
        /*
         * 构造函数，设置microSeconds值
         * microSecondsSinceEpoch：时间戳的微秒数
         * */
        explicit Timestamp(int64_t microSecondsSinceEpoch);

        /*
         * 交换两个timestamp对象的值
         * that：另一个timestamp对象地址
         * */
        void swap(Timestamp &that){
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

        /*
         * 用std::string形式返回，格式[millisec].[microsec]
         * */
        std::string toString() const;

        /*
         * 格式，"%4d年%02d月%02d日 星期%d %02d:%02d:%02d.%06d",时分秒.微秒
         * */
        std::string toFormattedString() const;

        /*
         * 判断当前时间是否合法，即是否>0
         * */
        bool valid() const { return microSecondsSinceEpoch_ > 0; }

        /*
         * 返回当前时间戳的微秒数
         * */
        int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
        /*
         * 返回当前时间戳的秒数
         * */
        //int64_t secSinceEpoch() const;

        /*
         * 返回当前时间所在的时间戳
         * */
        static Timestamp now();
        static Timestamp invalid();

        static const int64_t kMicroSecondsPerSecond = 1000 * 1000;  // 1秒=1000*1000微秒

    private:
        int64_t microSecondsSinceEpoch_;  // 数据成员，表示时间戳的微秒数
    };

    /*
     * 返回两个时间戳的比较值，自己实现<和==操作符的重载
     * */
    inline bool operator<(Timestamp lhs, Timestamp rhs){
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }
    inline bool operator==(Timestamp lhs, Timestamp rhs){
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    /*
     * 返回两个时间戳的差，用秒的形式返回
     * */
    inline double timeDifference(Timestamp high, Timestamp low){
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }
    /*
     * 返回一个时间戳加上给定秒数的新的时间戳
     * */
    inline Timestamp addTime(Timestamp timestamp, double seconds){
        int64_t delta = static_cast<int64_t>(seconds *Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }
}


#endif //MUDUO_BASE_TIMESTAMP_H
