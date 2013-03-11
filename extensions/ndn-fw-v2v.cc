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

#include "ndn-fw-v2v.h"
#include "ndn-v2v-net-device-face.h"
#include "geo-tag.h"

#include <ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h>

#include <ns3/log.h>
#include <ns3/ptr.h>
#include <ns3/assert.h>

#include <ns3/ndn-l3-protocol.h>
#include <ns3/ndn-pit-entry.h>
#include <ns3/ndn-fib-entry.h>
#include <ns3/ndn-interest.h>
#include <ns3/ndn-content-object.h>
#include <ns3/ndn-content-store.h>
#include <ns3/ndn-app-face.h>
#include <ns3/mobility-model.h>

#include <boost/foreach.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.fw.V2v");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (V2v);

TypeId
V2v::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::V2v")
    .SetParent<ForwardingStrategy> ()
    .SetGroupName ("Ndn")
    .AddConstructor<V2v> ()

  ;
  return tid;
}

V2v::V2v()
{
}

V2v::~V2v ()
{
}

void
V2v::OnInterest (Ptr<Face> face,
                 Ptr<const InterestHeader> header,
                 Ptr<const Packet> origPacket)
{
  if (DynamicCast<AppFace> (face))
    {
      Ptr<MobilityModel> model = GetObject<MobilityModel> ();
      Vector position;
      if (model)
        {
          position = model->GetPosition ();
          GeoSrcTag tag;
          tag.SetPosition (position);
          origPacket->AddPacketTag (tag);
        }
    }

  ForwardingStrategy::OnInterest (face, header, origPacket);
}

void
V2v::OnData (Ptr<Face> face,
             Ptr<const ContentObjectHeader> header,
             Ptr<Packet> payload,
             Ptr<const Packet> origPacket)
{
  if (DynamicCast<AppFace> (face))
    {
      Ptr<MobilityModel> model = GetObject<MobilityModel> ();
      Vector position;
      if (model)
        {
          position = model->GetPosition ();
          GeoSrcTag tag;
          tag.SetPosition (position);
          origPacket->AddPacketTag (tag);
          // payload->AddPacketTag (tag);
        }
    }

  ForwardingStrategy::OnData (face, header, payload, origPacket);
}


bool
V2v::DoPropagateInterest (Ptr<Face> inFace,
                          Ptr<const InterestHeader> header,
                          Ptr<const Packet> origPacket,
                          Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this);

  int propagatedCount = 0;

  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
      //if (metricFace.m_status == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in the front of the list
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in the front of the list
        break;

      //if (!TrySendOutInterest (inFace, metricFace.m_face, header, origPacket, pitEntry))
      if (!TrySendOutInterest (inFace, metricFace.GetFace (), header, origPacket, pitEntry))
        {
          continue;
        }

      propagatedCount++;
    }

  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}

void
V2v::DidReceiveSolicitedData (Ptr<Face> inFace,
                              Ptr<const ContentObjectHeader> header,
                              Ptr<const Packet> payload,
                              Ptr<const Packet> origPacket,
                              bool didCreateCacheEntry)
{
  ForwardingStrategy::DidReceiveSolicitedData (inFace, header, payload, origPacket, didCreateCacheEntry);

  if (didCreateCacheEntry)
    {
      // initiate low priority "pushing" only for "new data packets"

      Ptr<L3Protocol> l3 = this->GetObject<L3Protocol> ();
      NS_ASSERT (l3 != 0);

      // Push interest using low-priority send method
      for (uint32_t faceId = 0; faceId < l3->GetNFaces (); faceId++)
        {
          TrySendLowPriority (l3->GetFace (faceId), origPacket);
        }
    }
}

void
V2v::DidReceiveUnsolicitedData (Ptr<Face> inFace,
                                Ptr<const ContentObjectHeader> header,
                                Ptr<const Packet> payload,
                                Ptr<const Packet> origPacket,
                                bool didCreateCacheEntry)
{
  ForwardingStrategy::DidReceiveUnsolicitedData (inFace, header, payload, origPacket, didCreateCacheEntry);

  if (didCreateCacheEntry)
    {
      // initiate low priority "pushing" only for "new data packets"

      Ptr<L3Protocol> l3 = this->GetObject<L3Protocol> ();
      NS_ASSERT (l3 != 0);

      // Push interest using low-priority send method
      for (uint32_t faceId = 0; faceId < l3->GetNFaces (); faceId++)
        {
          TrySendLowPriority (l3->GetFace (faceId), origPacket);
        }
    }
}

void
V2v::DidExhaustForwardingOptions (Ptr<Face> inFace,
                                  Ptr<const InterestHeader> header,
                                  Ptr<const Packet> origPacket,
                                  Ptr<pit::Entry> pitEntry)
{
  ForwardingStrategy::DidExhaustForwardingOptions (inFace, header, origPacket, pitEntry);

  // Try to push new packet further with lowest priority possible on all L3 faces
  BOOST_FOREACH (const fib::FaceMetric &face, pitEntry->GetFibEntry ()->m_faces)
    {
      //TrySendLowPriority (face.m_face, origPacket);
      TrySendLowPriority (face.GetFace (), origPacket);
    }
}

void
V2v::TrySendLowPriority (Ptr<Face> face, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (boost::cref (*face));

  Ptr<V2vNetDeviceFace> v2vFace = DynamicCast<V2vNetDeviceFace> (face);
  if (v2vFace)
    {
      v2vFace->SendLowPriority (packet->Copy ());
    }
}


} // namespace fw
} // namespace ndn
} // namespace ns3
