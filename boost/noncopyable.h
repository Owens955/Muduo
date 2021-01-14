//
// Created by Kukai on 2020/12/7.
//

#ifndef MUDUO_BOOST_NONCOPYABLE_H
#define MUDUO_BOOST_NONCOPYABLE_H

namespace boost{
    namespace noncopyable_{
        /*
         * 禁止拷贝类
         * */
        class noncopyable {
        protected:
            noncopyable() = default;

            ~noncopyable() = default;

            noncopyable(const noncopyable &) = delete;

            noncopyable &operator=(const noncopyable &) = delete;
        };
    }
    typedef noncopyable_::noncopyable noncopyable;
}

#endif //MUDUO_BOOST_NONCOPYABLE_H
