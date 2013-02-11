/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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
 */

#ifndef NDN_FW_V2V_H
#define NDN_FW_V2V_H

#include "ns3/ndn-forwarding-strategy.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup Ndn
 * \brief Implementation of NDN forwarding strategy for V2V communications
 */
class V2v : public ForwardingStrategy
{
public:
  static TypeId GetTypeId ();

  /**
   * \brief Default constructor. Creates an empty stack without forwarding strategy set
   */
  V2v();
  virtual ~V2v ();

  // from ForwardingStrategy
  virtual void
  OnInterest (Ptr<Face> face,
              Ptr<const InterestHeader> header,
              Ptr<const Packet> origPacket);

  virtual void
  OnData (Ptr<Face> face,
          Ptr<const ContentObjectHeader> header,
          Ptr<Packet> payload,
          Ptr<const Packet> origPacket);

protected:
  // from ForwardingStrategy
  virtual bool
  DoPropagateInterest (Ptr<Face> inFace,
                       Ptr<const InterestHeader> header,
                       Ptr<const Packet> origPacket,
                       Ptr<pit::Entry> pitEntry);


  virtual void
  DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const InterestHeader> header,
                               Ptr<const Packet> origPacket,
                               Ptr<pit::Entry> pitEntry);

  virtual void
  DidReceiveSolicitedData (Ptr<Face> inFace,
                           Ptr<const ContentObjectHeader> header,
                           Ptr<const Packet> payload,
                           Ptr<const Packet> origPacket,
                           bool didCreateCacheEntry);

  virtual void
  DidReceiveUnsolicitedData (Ptr<Face> inFace,
                             Ptr<const ContentObjectHeader> header,
                             Ptr<const Packet> payload,
                             Ptr<const Packet> origPacket,
                             bool didCreateCacheEntry);
private:
  void
  TrySendLowPriority (Ptr<Face> face, Ptr<const Packet> packet);
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif /* CCNX_L3_PROTOCOL_H */
