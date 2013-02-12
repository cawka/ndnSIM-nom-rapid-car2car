/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef NDN_V2V_NET_DEVICE_FACE_H
#define NDN_V2V_NET_DEVICE_FACE_H

#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/random-variable.h"
#include "ns3/traced-callback.h"

#include "ns3/ndn-net-device-face.h"
#include "ns3/ndn-header-helper.h"

namespace ns3 {

class Vector3D;
typedef Vector3D Vector;

namespace ndn {

class Name;

/**
 * \ingroup ndn-face
 * \brief Implementation of layer-2 broadcast vehicle-2-vehicle NDN face
 *
 * This class defines basic functionality of NDN face. Face is core
 * component responsible for actual delivery of data packet to and
 * from NDN stack
 *
 * ndn::NetDevice face is permanently associated with one NetDevice
 * object and this object cannot be changed for the lifetime of the
 * face
 *
 * The only difference from the base class is that ndn::V2vNetDevice
 * makes additional consideration for overheard information
 *
 * \see ndn::AppFace, ndn::NetDeviceFace
 */
class V2vNetDeviceFace  : public NetDeviceFace
{
public:
  static TypeId
  GetTypeId ();

  /**
   * \brief Constructor
   *
   * \param netDevice a smart pointer to NetDevice object to which
   * this face will be associate
   */
  V2vNetDeviceFace (Ptr<Node> node, const Ptr<NetDevice> &netDevice);
  virtual ~V2vNetDeviceFace();

  // from CcnxFace
  virtual void
  SendLowPriority (Ptr<Packet> p);

  virtual void
  RegisterProtocolHandler (ProtocolHandler handler);

protected:
  // from ndn::NetDeviceFace
  virtual bool
  SendImpl (Ptr<Packet> p);

public:
  virtual std::ostream&
  Print (std::ostream &os) const;

private:
  V2vNetDeviceFace (const V2vNetDeviceFace &); ///< \brief Disabled copy constructor
  V2vNetDeviceFace& operator= (const V2vNetDeviceFace &); ///< \brief Disabled copy operator

  /// \brief callback from lower layers
  virtual void
  ReceiveFromNetDevice (Ptr<NetDevice> device,
                        Ptr<const Packet> p,
                        uint16_t protocol,
                        const Address &from,
                        const Address &to,
                        NetDevice::PacketType packetType);

  void
  SendFromQueue ();

  void
  SetMaxDelay (const Time &value);

  Time
  GetMaxDelay () const;

  void
  SetMaxDelayLowPriority (const Time &value);

  Time
  GetMaxDelayLowPriority () const;

  void
  ProcessRetx ();

  Time
  GetPriorityQueueGap () const;

  void
  NotifyJumpDistanceInterestTrace (const Ptr<const Packet> packet);

  void
  NotifyJumpDistanceDataTrace (const Ptr<const Packet> packet);

  void
  TagAndNetDeviceSendImpl (Ptr<Packet> packet);

private:
  struct Item
  {
    Item (const Time &_gap, const Ptr<Packet> &_packet);
    Item (const Item &item);

    Item &
    operator ++ ();

    Item &
    Gap (const Time &time);

    Time m_gap;
    Ptr<Packet> m_packet;
    HeaderHelper::Type m_type;
    Ptr<const Name> m_name;
    uint32_t m_retxCount;
  };
  typedef std::list<Item> ItemQueue;

  EventId m_scheduledSend;

  // Primary queue (for requested ContentObject packets)
  Time m_totalWaitPeriod;
  UniformVariable m_randomPeriod;
  Time m_maxWaitPeriod;
  uint32_t m_maxPacketsInQueue;
  ItemQueue m_queue;

  // Low-priority queue (for pushing Interest and ContentObject packets)
  Time m_maxWaitLowPriority;
  double m_maxDistance;
  ItemQueue m_lowPriorityQueue;

  // Retransmission queue for low-priority pushing
  EventId m_retxEvent;
  Time m_maxWaitRetransmission;
  ItemQueue m_retxQueue;
  uint32_t m_maxRetxAttempts;

  TracedCallback<double, double> m_waitingTimeVsDistanceDataTrace;
  TracedCallback<double, double> m_waitingTimeVsDistanceInterestTrace;

  TracedCallback<Ptr<const Node>, double> m_jumpDistanceDataTrace;
  TracedCallback<Ptr<const Node>, double> m_jumpDistanceInterestTrace;

  TracedCallback<Ptr<Node>, Ptr<const Packet>, const Vector&> m_txData;
  TracedCallback<Ptr<Node>, Ptr<const Packet>, const Vector&> m_txInterest;

  TracedCallback<Ptr<Node>, Ptr<const Packet> > m_cancellingData;
  TracedCallback<Ptr<Node>, Ptr<const Packet> > m_cancellingInterest;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_V2V_NET_DEVICE_FACE_H
