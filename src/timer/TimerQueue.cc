#include "TimerQueue.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include "Timer.h"
#include "TimerId.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>

//创建一个系统定时器，并返回其文件描述符
int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        //log
        printf("Failed in timerfd_create\n");
    }
    return timerfd;
}

timespec howMuchTimeFromNow(Timestamp when){
    Timestamp now(Timestamp::now());
    int64_t microSeconds = when-now;
    if(microSeconds < 100){
        microSeconds = 100;
    }
    timespec res;
    res.tv_sec = static_cast<time_t>(microSeconds / Timestamp::kMicroSecondsPerSecond);
    res.tv_nsec = static_cast<long>((microSeconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return res;
}

// 处理一次Timerfd超时事件，Timerfd超时，说明自定义的Timer也超时了，我们要处理的是自定义的Timer；对于Timerfd的超时，只需读一下，检查是否有错即可
void readTimerfd(int timerfd, Timestamp now){
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    //log
    printf("TimerQueue::handleRead()%s at \n",now.toString().data());
    if (n != sizeof howmany) {
        printf("TimerQueue::handleRead() reads %d  bytes instead of 8\n",n);
    }
}

// 根据Timer最近的超时时刻，重新设置Timerfd的超时时间，当Timerfd超时时，必有至少一个Timer超时，随即进行处理
void resetTimerfd(int timerfd, Timestamp expiration){
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    //设置系统定时器超时的时间间隔
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret){ 
        //log
        printf("timerfd_settime() failed()\n");
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_){
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue(){
    // 关闭定时器所关联的那个通道的所有事件, Poller不再监听该通道
    timerfdChannel_.disableAll();
    // 如果正在处理该通道, 会从激活的通道列表中移除, 同时Poller不再监听该通道
    timerfdChannel_.remove();
    // 关闭通道对应timerfd
    ::close(timerfd_);

    while (!timers_.empty()) {
        const Entry& timer = timers_.top();
        timers_.pop();
        delete timer.second;
    }
}

/**
 * 添加一个定时器.
 * @details 运行到指定时间点when, 调度相应的回调函数cb.
 * 如果interval参数 > 0.0, 就周期重复运行.
 * 可能会由其他线程调用, 需要让对TimerQueue数据成员有修改的部分, 在所属loop所在线程中运行.
 * @param cb 超时回调函数
 * @param when 触发超时的时间点
 * @param interval 循环周期. > 0.0 代表周期定时器; 否则, 代表一次性定时器
 * @return 返回添加的Timer对应TimerId, 用来标识该Timer对象
 */
void TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addtimerInLoop, this, timer));
}

void TimerQueue::addtimerInLoop(Timer* timer) {
    // 新增的定时器的到期时间是否是最早的
    bool earliestChanged = insert(timer);
    if (earliestChanged) {
        // 最近的Timer超时时刻发生了改变，需重新设置timerfd，timerfd的超时时刻应始终为最近的Timer超时时刻
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 把Timer对象加入到TimerQueue中
bool TimerQueue::insert(Timer* timer){
    bool earliestChanged = false;
    Timestamp when = timer->expiration();  //当前Timer超时时刻
    const Entry queueTop = timers_.top();
    if (timer->expiration() < queueTop.first) {
        earliestChanged = true;
    }
    timers_.push({timer->expiration(), timer});
    return earliestChanged;
}

/**
 * 处理读事件, 只能是所属loop线程调用
 * @details 当Poller的poll监听到超时发生时, 将channel加入激活通道列表, loop中回调
 * 事件处理函数, TimerQueue::handleRead.
 * 发生超时事件时, 可能会有多个超时任务超时, 需要通过getExpired一次性全部获取, 然后逐个执行回调.
 * @note timerfd只会发生读事件.
 */
void TimerQueue::handleRead() {
    Timestamp now(Timestamp::now());
    //先处理Timerfd的超时事件，再处理自定义的Timer的超时
    readTimerfd(timerfd_, now);

    // 获取当前所有超时的定时器
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    for(const Entry& it : expired){
        it.second->run(); //通过Timer::run()回调超时处理函数
    }
    callingExpiredTimers_ = false;
    // 重置所有已超时的定时器
    reset(expired,now);
}

/**
 * 定时任务超时时, 从timers_中取出所有的超时任务, 以vector形式返回给调用者
 * @param now 当前时间点, 用来判断定时器是否超时
 * @return vector timers_中超时的定时器
 */
std::vector<Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;
    while (!timers_.empty()) {
        const Entry queueTop = timers_.top();
        timers_.pop();
        if (queueTop.first <= now) {
            expired.push_back(queueTop);
        } else {
            break;
        }
    }
    return expired;
}

/**
 * 根据指定时间now重置所有超时任务, 只对周期定时任务有效
 * @param expired 所有超时任务
 * @param now 指定的reset基准时间点, 新的超时时间点以此为基准
 */
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now){
    Timestamp nextExpire;
    for(const Entry& it : expired){
        // 重复任务则继续执行
        if (it.second->repeat()) {
            it.second->restart(now);
            // 把重置后的定时器添加回timers_
            insert(it.second);
        }else{
            delete it.second;
        }
    }
    // 如果重新插入了定时器，需要继续重置timerfd
    if (!timers_.empty()) {
        resetTimerfd(timerfd_, timers_.top().first);
    }
}