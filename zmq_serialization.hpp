#pragma once

#include <zmq.hpp>

namespace {
  using DomainIdentity_t = icon::details::fields2::Identity;
  using TransportIdentity_t = zmq::message_t;
}

namespace icon::details::serialization
{
template<class RawData>
class ZmqData
{
public:
  ZmqData() = default;
  explicit ZmqData(RawData data)
    : data_{std::move(data)}
  {};

  ZmqData(const ZmqData&) = delete;
  ZmqData& operator=(const ZmqData&) = delete;
  ZmqData(ZmqData&&) = default;
  ZmqData& operator=(ZmqData&&) = default;

  bool has_value() const
  {
    return data_.has_value();
  }

  const RawData& data() const
  {
    return *data_;
  }

  std::optional<RawData> data_;
};

template<class Destination>
Destination deserialize(const ZmqData<TransportIdentity_t>& src_identity) requires std::is_same_v<Destination, DomainIdentity_t>
{
  const auto& src_identity_data = src_identity.data();
  
  return DomainIdentity_t{src_identity_data.to_string()};
}

}
