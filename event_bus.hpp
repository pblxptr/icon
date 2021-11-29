#pragma once

#include <boost/signals2/signal.hpp>

template<class TEvent>
class Observable
{
  using Slot_t = boost::signals2::signal<void(const TEvent&)>;
public:
  using Subscriber_t = typename Slot_t::slot_type;

  void publish(const TEvent& event)
  {
    slot_(event);
  }

  void subscribe(const Subscriber_t& subscriber)
  {
    slot_.connect(subscriber);
  }
private:
  Slot_t slot_;
};

template<class... TEvents>
class EventBus : public Observable<TEvents>...
{
public:
  template<class TEvent>
  void publish(const TEvent& event)
  {
    observable<TEvent>().publish(event);
  }

  template<class TEvent, class TSubscriber>
  void subscribe(const TSubscriber& subscriber)
  {
    observable<TEvent>().subscribe(subscriber);
  }
private:
  template<class TEvent>
  auto& observable()
  {
    return static_cast<Observable<TEvent>&>(*this);
  }
};