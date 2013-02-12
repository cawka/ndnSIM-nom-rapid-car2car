/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2013 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *
 */

#include "ndn-v2v-net-device-face.h"
#include "geo-tag.h"

#include "ns3/ndn-l3-protocol.h"
#include "ns3/ndn-header-helper.h"

#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/ndn-name-components.h"

NS_LOG_COMPONENT_DEFINE ("ndn.V2vNetDeviceFace");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (V2vNetDeviceFace);

TypeId
V2vNetDeviceFace::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::V2vNetDeviceFace")
    .SetParent<NetDeviceFace> ()
    .SetGroupName ("Ccnx")

    .AddAttribute ("MaxDelay", "Maximum delay that packets should wait in queue (max for uniform random variable)",
                   TimeValue (Seconds (0.002)),
                   MakeTimeAccessor (&V2vNetDeviceFace::SetMaxDelay, &V2vNetDeviceFace::GetMaxDelay),
                   MakeTimeChecker ())

    .AddAttribute ("MaxDelayLowPriority", "Maximum delay for low-priority queue ('gradient' pushing queue)",
                   TimeValue (Seconds (0.010)),
                   MakeTimeAccessor (&V2vNetDeviceFace::SetMaxDelayLowPriority, &V2vNetDeviceFace::GetMaxDelayLowPriority),
                   MakeTimeChecker ())
    .AddAttribute ("MaxDistance", "Normalization distance for gradient pushing calculation",
                   DoubleValue (300.0),
                   MakeDoubleAccessor (&V2vNetDeviceFace::m_maxDistance),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("MaxDelayRetransmission", "Maximum delay between successive retransmissions of low-priority pushed packets",
                   TimeValue (Seconds (0.050)),
                   MakeTimeAccessor (&V2vNetDeviceFace::m_maxWaitRetransmission),
                   MakeTimeChecker ())
    .AddAttribute ("MaxRetransmissionAttempts", "Maximum number of retransmission attempts for low-priority pushed packets",
                   UintegerValue (7),
                   MakeUintegerAccessor (&V2vNetDeviceFace::m_maxRetxAttempts),
                   MakeUintegerChecker<uint32_t> ())

    .AddTraceSource ("WaitingTimeVsDistanceDataTrace", "On every low-priority packet trace the waiting gap and distance",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_waitingTimeVsDistanceDataTrace))
    .AddTraceSource ("WaitingTimeVsDistanceInterestTrace", "On every low-priority packet trace the waiting gap and distance",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_waitingTimeVsDistanceInterestTrace))

    .AddTraceSource ("JumpDistanceData", "Fired just before packet is actually transmitted if GeoTag is present and distance is more than 0",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_jumpDistanceDataTrace))
    .AddTraceSource ("JumpDistanceInterest", "Fired just before packet is actually transmitted if GeoTag is present and distance is more than 0",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_jumpDistanceInterestTrace))

    .AddTraceSource ("TxData", "Fired every time packet is send out of face",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_txData))
    .AddTraceSource ("TxInterest", "Fired every time packet is send out of face",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_txInterest))

    .AddTraceSource ("CancelingData", "Fired every time transmission is cancelled",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_cancellingData))
    .AddTraceSource ("CancelingInterest", "Fired every time transmission is cancelled",
                     MakeTraceSourceAccessor (&V2vNetDeviceFace::m_cancellingInterest))
    ;
  return tid;
}

/**
 * By default, Ccnx face are created in the "down" state.  Before
 * becoming useable, the user must invoke SetUp on the face
 */
V2vNetDeviceFace::V2vNetDeviceFace (Ptr<Node> node, const Ptr<NetDevice> &netDevice)
  : NetDeviceFace (node, netDevice)
  , m_maxPacketsInQueue (100)
{
  NS_LOG_FUNCTION (this << node << netDevice);

  PointerValue pointer;
  bool ok = netDevice->GetAttributeFailSafe ("Mac", pointer);
  if (!ok)
    {
      NS_LOG_DEBUG ("Cannot get `Mac` thing from NetDevice");
      return;
    }
  pointer.Get<Object> () ->GetAttributeFailSafe ("DcaTxop", pointer);
  if (!ok)
    {
      NS_LOG_DEBUG ("Cannot get `DcaTxop` thing from WifiMac");
      return;
    }
  pointer.Get<Object> () ->GetAttributeFailSafe ("Queue", pointer);
  if (!ok)
    {
      NS_LOG_DEBUG ("Cannot get `Queue` thing from DcaTxop");
      return;
    }

  UintegerValue value;
  pointer.Get<Object> () ->GetAttributeFailSafe ("MaxPacketNumber", value);
  if (!ok)
    {
      NS_LOG_DEBUG ("Cannot get `MaxPacketNumber` thing from Queue");
      return;
    }

  NS_LOG_DEBUG ("Number of packets obtained from WifiMac/DcaTxop/Queue: " << value.Get ());
  m_maxPacketsInQueue = value.Get ();
}

V2vNetDeviceFace::~V2vNetDeviceFace ()
{
  NS_LOG_FUNCTION (this);
}

V2vNetDeviceFace& V2vNetDeviceFace::operator= (const V2vNetDeviceFace &)
{
  return *this;
}

void
V2vNetDeviceFace::SetMaxDelay (const Time &value)
{
  NS_LOG_FUNCTION (this << value);

  m_maxWaitPeriod = value;
  m_randomPeriod = UniformVariable (0, m_maxWaitPeriod.ToDouble (Time::S));
}

Time
V2vNetDeviceFace::GetMaxDelay () const
{
  return m_maxWaitPeriod;
}

void
V2vNetDeviceFace::SetMaxDelayLowPriority (const Time &value)
{
  m_maxWaitLowPriority = value;
}

Time
V2vNetDeviceFace::GetMaxDelayLowPriority () const
{
  return m_maxWaitLowPriority;
}


V2vNetDeviceFace::Item::Item (const Time &gap, const Ptr<Packet> &packet)
  : m_gap (gap), m_packet (packet), m_retxCount (0)
{
  // NS_LOG_FUNCTION (this << _gap << _packet);

  m_name = HeaderHelper::GetName (packet); // not efficient (extra deserialization), but it is the only way for now
  NS_ASSERT (m_name != 0);

  HeaderHelper::Type guessedType = HeaderHelper::GetNdnHeaderType (packet);
  if (guessedType == HeaderHelper::INTEREST_CCNB ||
      guessedType == HeaderHelper::INTEREST_NDNSIM)
    {
      m_type = HeaderHelper::INTEREST_NDNSIM;
      NS_LOG_DEBUG ("Schedule low-priority Interest");
    }
  else if (guessedType == HeaderHelper::CONTENT_OBJECT_CCNB ||
           guessedType == HeaderHelper::CONTENT_OBJECT_NDNSIM)
    {
      m_type = HeaderHelper::CONTENT_OBJECT_NDNSIM;
      NS_LOG_DEBUG ("Schedule low-priority ContentObject");
    }
  else
    {
      NS_FATAL_ERROR ("Unknown NDN header type");
    }
}

V2vNetDeviceFace::Item::Item (const Item &item)
  : m_gap (item.m_gap), m_packet (item.m_packet), m_type (item.m_type), m_name (item.m_name), m_retxCount (item.m_retxCount)
{
}

V2vNetDeviceFace::Item &
V2vNetDeviceFace::Item::operator ++ ()
{
  // remote GeoTag when packet is scheduled for retransmission
  GeoTransmissionTag tag;
  m_packet->RemovePacketTag (tag);

  m_retxCount ++;
  return *this;
}

V2vNetDeviceFace::Item &
V2vNetDeviceFace::Item::Gap (const Time &time)
{
  m_gap = time;
  return *this;
}

Time
V2vNetDeviceFace::GetPriorityQueueGap () const
{
  Time gap = Seconds (m_randomPeriod.GetValue ());
  if (m_totalWaitPeriod < m_maxWaitPeriod)
    {
      gap = std::min (m_maxWaitPeriod - m_totalWaitPeriod, gap);
    }
  else
    gap = Time (0);

  return gap;
}

void
V2vNetDeviceFace::SendLowPriority (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  if (m_queue.size () >= m_maxPacketsInQueue ||
      m_lowPriorityQueue.size () >= m_maxPacketsInQueue)
    {
      // \todo Maybe add tracing
      NS_LOG_DEBUG ("Too many packets enqueue already. Don't do anything");
      return;
    }

  GeoTransmissionTag tag;
  bool isTag = packet->PeekPacketTag (tag);

  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  if (mobility == 0)
    {
      NS_FATAL_ERROR ("Mobility model has to be installed on the node");
      return;
    }

  double distance = m_maxDistance;
  if (isTag) // if !isTag, it means that packet came from application
    {
      // NS_LOG_DEBUG ("Tag is OK, distance is " << CalculateDistance (tag->GetPosition (), mobility->GetPosition ()));
      distance = CalculateDistance (tag.GetPosition (), mobility->GetPosition ());
      distance = std::min (m_maxDistance, distance);
    }


  //////////////////////////////////
  //////////////////////////////////

  // Mean waiting time.  Reversely proportional to the distance from the original transmitter
  // Closer guys will tend to wait longer than guys far away
  double meanWaiting = m_maxWaitLowPriority.ToDouble (Time::S) * (m_maxDistance - distance) / m_maxDistance;

  //////////////////////////////////
  //////////////////////////////////

  UniformVariable randomLowPriority (meanWaiting, meanWaiting + m_maxWaitPeriod.ToDouble (Time::S));

  double sample = std::abs (randomLowPriority.GetValue ());
  // NS_LOG_DEBUG ("Sample: " << sample);

  Item queueItem (Seconds (sample), packet);
  if (queueItem.m_type == HeaderHelper::INTEREST_NDNSIM)
    {
      m_waitingTimeVsDistanceInterestTrace (distance, sample);
    }
  else
    {
      m_waitingTimeVsDistanceDataTrace (distance, sample);
    }

  // Actual gap will be defined by Triangular distribution based on Geo metric + Uniform distribution that is aimed to avoid collisions
  m_lowPriorityQueue.push_back (queueItem);

  if (!m_scheduledSend.IsRunning ())
    m_scheduledSend = Simulator::Schedule (m_lowPriorityQueue.front ().m_gap, &V2vNetDeviceFace::SendFromQueue, this);
}

bool
V2vNetDeviceFace::SendImpl (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  HeaderHelper::Type type = HeaderHelper::GetNdnHeaderType (packet);
  if (type == HeaderHelper::INTEREST_CCNB ||
      type == HeaderHelper::INTEREST_NDNSIM)
    {
      // send immediately, don't delay

      //////////////////////////////
      return NetDeviceFace::SendImpl (packet);
      //////////////////////////////
    }
  else if (type == HeaderHelper::CONTENT_OBJECT_CCNB ||
           type == HeaderHelper::CONTENT_OBJECT_NDNSIM)
    {
      if (m_queue.size () >= m_maxPacketsInQueue)
        {
          // \todo Maybe add tracing
          NS_LOG_INFO ("Dropping data packet that exceed queue size");
          return false;
        }

      Time gap = GetPriorityQueueGap ();
      m_queue.push_back (Item (gap, packet));
      m_totalWaitPeriod += gap;

      if (!m_scheduledSend.IsRunning ())
        m_scheduledSend = Simulator::Schedule (m_queue.front ().m_gap, &V2vNetDeviceFace::SendFromQueue, this);

      return true;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown or unsupported NDN header type");
      return false;
    }
}

void
V2vNetDeviceFace::NotifyJumpDistanceInterestTrace (Ptr<const Packet> packet)
{
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  if (mobility == 0)
    {
      NS_FATAL_ERROR ("Mobility model has to be installed on the node");
      return;
    }

  GeoTransmissionTag tag;
  bool isTag = packet->PeekPacketTag (tag);

  if (!isTag) return;

  double distance = CalculateDistance (tag.GetPosition (), mobility->GetPosition ());

  m_jumpDistanceInterestTrace (m_node, distance);
}

void
V2vNetDeviceFace::NotifyJumpDistanceDataTrace (Ptr<const Packet> packet)
{
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  if (mobility == 0)
    {
      NS_FATAL_ERROR ("Mobility model has to be installed on the node");
      return;
    }

  GeoTransmissionTag tag;
  bool isTag = packet->PeekPacketTag (tag);

  if (!isTag) return;

  double distance = CalculateDistance (tag.GetPosition (), mobility->GetPosition ());

  m_jumpDistanceDataTrace (m_node, distance);
}

void
V2vNetDeviceFace::SendFromQueue ()
{
  // NS_LOG_FUNCTION (this);

  NS_ASSERT ((m_queue.size () + m_lowPriorityQueue.size ()) > 0);

  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  if (mobility == 0)
    {
      NS_FATAL_ERROR ("Mobility model has to be installed on the node");
      return;
    }

  // If high-priority queue is not empty, send data from it
  if (m_queue.size () > 0)
    {
      Item &item = m_queue.front ();

      //////////////////////////////
      TagAndNetDeviceSendImpl (item.m_packet->Copy ());
      if (item.m_type == HeaderHelper::INTEREST_NDNSIM)
        {
          m_txInterest (m_node, item.m_packet, mobility->GetPosition ());
        }
      else
        {
          m_txData (m_node, item.m_packet, mobility->GetPosition ());
        }
      //////////////////////////////

      if (item.m_retxCount < m_maxRetxAttempts)
        m_retxQueue.push_back (++(item.Gap (m_maxWaitRetransmission)));

      m_totalWaitPeriod -= item.m_gap;
      m_queue.pop_front ();
    }
  else if (m_lowPriorityQueue.size () > 0) // no reason for this check, just for readability
    {
      Item &item = m_lowPriorityQueue.front ();

      //////////////////////////////
      TagAndNetDeviceSendImpl (item.m_packet->Copy ());
      if (item.m_type == HeaderHelper::INTEREST_NDNSIM)
        {
          m_txInterest (m_node, item.m_packet, mobility->GetPosition ());
        }
      else
        {
          m_txData (m_node, item.m_packet, mobility->GetPosition ());
        }
      //////////////////////////////

      if (item.m_retxCount < m_maxRetxAttempts)
        m_retxQueue.push_back (++(item.Gap (m_maxWaitRetransmission)));

      m_lowPriorityQueue.pop_front ();
    }

  if (m_queue.size () > 0)
    m_scheduledSend = Simulator::Schedule (m_queue.front ().m_gap, &V2vNetDeviceFace::SendFromQueue, this);
  else if (m_lowPriorityQueue.size () > 0)
    m_scheduledSend = Simulator::Schedule (m_lowPriorityQueue.front ().m_gap, &V2vNetDeviceFace::SendFromQueue, this);

  if (!m_retxEvent.IsRunning () && m_retxQueue.size () > 0)
    {
      m_retxEvent = Simulator::Schedule (m_retxQueue.front ().m_gap, &V2vNetDeviceFace::ProcessRetx, this);
    }
}

void
V2vNetDeviceFace::TagAndNetDeviceSendImpl (Ptr<Packet> packet)
{
  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();
  if (mobility != 0)
    {
      GeoTransmissionTag tag;
      packet->RemovePacketTag (tag);

      tag.SetPosition (mobility->GetPosition ());
      packet->AddPacketTag (tag);
    }

  NetDeviceFace::SendImpl (packet);
}


void
V2vNetDeviceFace::ProcessRetx ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_retxQueue.size () > 0);

  Time gap = GetPriorityQueueGap ();
  Item item (gap, m_retxQueue.front ().m_packet);
  item.m_retxCount = m_retxQueue.front ().m_retxCount;
  m_lowPriorityQueue.push_back (item);

  m_retxQueue.pop_front ();

  if (!m_scheduledSend.IsRunning ())
    m_scheduledSend = Simulator::Schedule (m_lowPriorityQueue.front ().m_gap, &V2vNetDeviceFace::SendFromQueue, this);

  if (m_retxQueue.size () > 0)
    m_retxEvent = Simulator::Schedule (m_retxQueue.front ().m_gap, &V2vNetDeviceFace::ProcessRetx, this);
}

void
V2vNetDeviceFace::RegisterProtocolHandler (ProtocolHandler handler)
{
  NS_LOG_FUNCTION (this);

  Face::RegisterProtocolHandler (handler);

  m_node->RegisterProtocolHandler (MakeCallback (&V2vNetDeviceFace::ReceiveFromNetDevice, this),
                                   L3Protocol::ETHERNET_FRAME_TYPE, GetNetDevice (), true/*promiscuous mode*/);
}


// callback
void
V2vNetDeviceFace::ReceiveFromNetDevice (Ptr<NetDevice>,
                                        Ptr<const Packet> p,
                                        uint16_t,
                                        const Address &,
                                        const Address &,
                                        NetDevice::PacketType)
{
  // NS_LOG_FUNCTION (this << p);

  HeaderHelper::Type packetType = HeaderHelper::GetNdnHeaderType (p);
  if (packetType == HeaderHelper::INTEREST_CCNB ||
      packetType == HeaderHelper::INTEREST_NDNSIM)
    {
      packetType = HeaderHelper::INTEREST_NDNSIM;
    }
  else if (packetType == HeaderHelper::CONTENT_OBJECT_CCNB ||
           packetType == HeaderHelper::CONTENT_OBJECT_NDNSIM)
    {
      packetType = HeaderHelper::CONTENT_OBJECT_NDNSIM;
    }
  else
    {
      NS_FATAL_ERROR ("Unknown NDN header type");
    }

  Ptr<const Name> name = HeaderHelper::GetName (p); // not efficient (extra deserialization), but it is the only way for now
  NS_ASSERT (name != 0);

  GeoSrcTag srcTag;
  GeoTransmissionTag transmissionTag;

  bool isSrcTag = p->PeekPacketTag (srcTag);
  bool isTransmissionTag = p->PeekPacketTag (transmissionTag);

  Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel> ();

  NS_LOG_DEBUG ("SANITY CHECK: " << isSrcTag << ", " << isTransmissionTag << ", " << mobility);

  //   src  -----   <transmission>  ---- <mobility>
  bool needToCancel = true;
  if (mobility && isSrcTag && isTransmissionTag)
    {
      NS_LOG_DEBUG ("Check distances:" << CalculateDistance (srcTag.GetPosition (), transmissionTag.GetPosition ()) << " <? " << CalculateDistance (srcTag.GetPosition (), mobility->GetPosition ()));
      if (CalculateDistance (srcTag.GetPosition (), transmissionTag.GetPosition ())
          <
          CalculateDistance (srcTag.GetPosition (), mobility->GetPosition ()))
        {
          needToCancel = false;
        }
    }

  bool cancelled = false;
  ItemQueue::iterator item = m_lowPriorityQueue.begin ();
  while (item != m_lowPriorityQueue.end ())
    {
      NS_ASSERT (item->m_name != 0);

      if ((packetType==HeaderHelper::CONTENT_OBJECT_NDNSIM || item->m_type == packetType) && *item->m_name == *name)
        {
          cancelled = item->m_type == packetType;

          if (needToCancel)
            {
              ItemQueue::iterator tmp = item;
              tmp ++;

              NS_LOG_INFO ("Canceling ContentObject with name " << name->GetLastComponent () << ", which is scheduled for low-priority transmission");
              m_cancellingData (m_node, item->m_packet);

              m_lowPriorityQueue.erase (item);
              if (m_queue.size () + m_lowPriorityQueue.size () == 0)
                {
                  Simulator::Remove (m_scheduledSend);
                }

              item = tmp;
            }
          else
            {
              item ++;
            }
        }
      else
        item ++;
    }

  item = m_queue.begin ();
  while (item != m_queue.end ())
    {
      NS_ASSERT (item->m_name != 0);

      if ((packetType==HeaderHelper::CONTENT_OBJECT_NDNSIM || item->m_type == packetType) && *item->m_name == *name)
        {
          cancelled = item->m_type == packetType;

          if (needToCancel)
            {
              ItemQueue::iterator tmp = item;
              tmp ++;

              NS_LOG_INFO ("Canceling ContentObject with name " << name->GetLastComponent () << ", which is scheduled for transmission");
              m_cancellingData (m_node, item->m_packet);

              m_totalWaitPeriod -= item->m_gap;
              m_queue.erase (item);
              if (m_queue.size () == 0)
                {
                  Simulator::Remove (m_scheduledSend);
                }

              item = tmp;
            }
          else
            item ++;
        }
      else
        item ++;
    }

  item = m_retxQueue.begin ();
  while (item != m_retxQueue.end ())
    {
      NS_ASSERT (item->m_name != 0);

      if ((packetType==HeaderHelper::CONTENT_OBJECT_NDNSIM || item->m_type == packetType) && *item->m_name == *name)
        {
          cancelled = item->m_type == packetType;
          if (needToCancel)
            {
              ItemQueue::iterator tmp = item;
              tmp ++;

              NS_LOG_INFO ("Canceling ContentObject with name " << name->GetLastComponent () << ", which is planned for retransmission");
              m_cancellingData (m_node, item->m_packet);

              m_retxQueue.erase (item);
              if (m_retxQueue.size () == 0)
                {
                  NS_LOG_INFO ("Canceling the retx processing event");
                  Simulator::Remove (m_retxEvent);
                }

              item = tmp;
            }
          else
            item ++;
        }
      else
        item ++;
    }

  if (cancelled)
    {
      if (!needToCancel)
        {
          NS_LOG_DEBUG ("Ignoring cancellation from a backwards node");
        }
      NS_LOG_DEBUG ("Cancelled");
      return;
    }
  else{
    if (packetType==HeaderHelper::INTEREST_NDNSIM)
      {
        NotifyJumpDistanceInterestTrace (p);
      }
    else
      {
        NotifyJumpDistanceDataTrace (p);
      }
    Receive (p);
  }
}


std::ostream&
V2vNetDeviceFace::Print (std::ostream& os) const
{
  os << "dev=v2v-net(" << GetId () << ")";
  return os;
}

} // namespace ndn
} // namespace ns3
