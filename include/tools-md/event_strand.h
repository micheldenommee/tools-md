/*
MIT License

Copyright (c) 2011-2019 Michel Dénommée

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _tools_md_event_strand_h
#define _tools_md_event_strand_h

#include "event_queue.h"

#define MD_STRAND_TO_TASKBASE(x)  \
std::static_pointer_cast<md::event_task_base>( \
    std::static_pointer_cast<md::event_strand<T>>(x) \
)

namespace md{

template<typename T = int>
class event_strand
    : public event_queue, public event_task_base//, 
    //public std::enable_shared_from_this< event_strand<T> >
{
    friend class md::event_queue;

    template< typename Task >
    friend uint64_t _event_strand_push_back(event_queue* eq, Task task);
    friend uint64_t _event_strand_push_back(
        event_queue* eq, sp_event_task task
    );

    template< typename Task >
    friend uint64_t _event_strand_push_front(event_queue* eq, Task task);
    friend uint64_t _event_strand_push_front(
        event_queue* eq, sp_event_task task
    );
    
public:
    event_strand(bool auto_requeue = true)
        : event_task_base(event_queue::get_default()),
        _auto_requeue(auto_requeue)
    {
    }
    
    event_strand(event_queue* owner, bool auto_requeue = true)
        : event_task_base(owner), _auto_requeue(auto_requeue)
    {
    }
    
    virtual ~event_strand()
    {
        for(size_t i = 0; i < _tasks.size(); ++i){
            _tasks[i]->_owner = this->_owner;
            this->_owner->push_back(_tasks[i]);
        }
    }
    
    virtual bool force_push(){ return true;}
    virtual size_t size() const
    {
        return _tasks.size();
    }
    
    virtual event_requeue_pos requeue()
    {
        if(_auto_requeue && this->size() > 0)
            return event_requeue_pos::back;
        
        return event_requeue_pos::none;
    }
    
    T data(){ return _data;}
    void data(T val){ _data = val;}
    
    template< typename Task >
    uint64_t push_back(Task task)
    {
        MD_LOCK_EVENT_QUEUE;
        
        auto id = _event_queue_push_back(this, task);
        
        if(_auto_requeue)
            this->_owner->push_back(
                MD_STRAND_TO_TASKBASE(this->shared_from_this())
            );
        return id;
    }
    
    template< typename Task >
    uint64_t push_front(Task task)
    {
        MD_LOCK_EVENT_QUEUE;

        auto id = _event_queue_push_front(this, task);
        if(_auto_requeue)
            this->_owner->push_back(
                MD_STRAND_TO_TASKBASE(this->shared_from_this())
            );
        return id;
    }
    
    virtual void run_task()
    {
        this->run_n();
    }
    
    void requeue_self_back()
    {
        this->_owner->push_back(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
    }
    
    void requeue_self_front()
    {
        this->_owner->push_front(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
    }
    
private:
    #ifdef MD_THREAD_SAFE
    mutable std::mutex _mutex;
    #endif

    bool _auto_requeue;
    T _data;
};

/*
template <>
uint64_t event_strand::push_back(sp_event_task tp_task)
{
    _evq_lock lock(_mutex, _is_thread_safe.load(std::memory_order_acquire));
    
    if(tp_task->_owner != this)
        throw md::exception(
            "Can't requeue a task created on another event_queue!\n"
            "Call the event_task::switch_owner function instead."
        );
    
    if(tp_task->force_push()){
        _tasks.emplace_back(tp_task);
        this->_owner->push_back(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
        return tp_task->id();
    }
    
    auto it = std::find_if(
        _tasks.begin(),
        _tasks.end(),
        [task_id=tp_task->id()](const sp_event_task& t)-> bool {
            return t->id() == task_id;
        }
    );
    
    if(it != _tasks.end())
        return tp_task->id();
    _tasks.emplace_back(tp_task);
    this->_owner->push_back(
        MD_STRAND_TO_TASKBASE(this->shared_from_this())
    );
    return tp_task->id();
}

template <>
uint64_t event_strand::push_front(sp_event_task tp_task)
{
    _evq_lock lock(_mutex, _is_thread_safe.load(std::memory_order_acquire));

    if(tp_task->_owner != this)
        throw md::exception(
            "Can't requeue a task created on another event_queue!\n"
            "Call the event_task::switch_owner function instead."
        );
    
    if(tp_task->force_push()){
        _tasks.emplace_back(tp_task);
        this->_owner->push_back(
            MD_STRAND_TO_TASKBASE(this->shared_from_this())
        );
        return tp_task->id();
    }
        
    auto it = std::find_if(
        _tasks.begin(),
        _tasks.end(),
        [task_id=tp_task->id()](const sp_event_task& t)-> bool {
            return t->id() == task_id;
        }
    );
    
    if(it != _tasks.end())
        return tp_task->id();
    _tasks.emplace_front(tp_task);
    this->_owner->push_back(
        MD_STRAND_TO_TASKBASE(this->shared_from_this())
    );
    return tp_task->id();
}
*/




}//::md
#endif //_tools_md_event_strand_h
