/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
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
 */

#include "car-relay-tracer.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-model.h"

#include "ns3/ndn-l3-protocol.h"
#include "ns3/ndn-content-store.h"
#include "ns3/ndn-app.h"
#include "ns3/ndn-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-content-object.h"

#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

NS_LOG_COMPONENT_DEFINE ("CarRelayTracer");

using namespace boost;
using namespace std;

namespace ns3 {
namespace ndn {

boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<CarRelayTracer> > >
CarRelayTracer::InstallAll (const std::string &file, int types)
{
  std::list<boost::shared_ptr<CarRelayTracer> > tracers;
  boost::shared_ptr<std::ofstream> outputStream (new std::ofstream ());
  outputStream->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

  if (!outputStream->is_open ())
    return boost::make_tuple (outputStream, tracers);

  for (NodeList::Iterator node = NodeList::Begin ();
       node != NodeList::End ();
       node++)
    {
      NS_LOG_DEBUG ("Node: " << lexical_cast<string> ((*node)->GetId ()));

      boost::shared_ptr<CarRelayTracer> trace = make_shared<CarRelayTracer> (outputStream, *node);
      if (types & DISTANCE_WAITING)
        trace->ConnectDistance ();

      if (types & JUMP_DISTANCE)
        trace->ConnectJumpDistance ();

      if (types & TX)
        trace->ConnectTx ();

      if (types & IN_CACHE)
        trace->ConnectInCache ();

      tracers.push_back (trace);
    }

  if (!tracers.empty ())
    {
      if (types & DISTANCE_WAITING)
        *outputStream << "Time\tType\tJumpDistance\tWaiting\n";

      if (types & JUMP_DISTANCE)
        *outputStream << "Time\tNodeId\tJumpDistance\n";

      if (types & TX)
        *outputStream << "Time\tNodeId\tX\tY\tZ\n";

      if (types & IN_CACHE)
        *outputStream << "Time\tNodeId\tX\tY\tZ\n";
    }

  return boost::make_tuple (outputStream, tracers);
}

CarRelayTracer::CarRelayTracer (boost::shared_ptr<std::ostream> os, Ptr<Node> node)
  : m_nodePtr (node)
  , m_os (os)
{
  m_node = boost::lexical_cast<string> (m_nodePtr->GetId ());

  string name = Names::FindName (node);
  if (!name.empty ())
    {
      m_node = name;
    }
}

void
CarRelayTracer::ConnectDistance ()
{
  Ptr<L3Protocol> ndn = m_nodePtr->GetObject<L3Protocol> ();
  for (uint32_t faceId = 0; faceId < ndn->GetNFaces (); faceId++)
    {
      ndn->GetFace (faceId)->TraceConnectWithoutContext ("WaitingTimeVsDistanceDataTrace", MakeCallback (&CarRelayTracer::DistanceVsWaiting, this));
    }
}

void
CarRelayTracer::ConnectJumpDistance ()
{
  Ptr<L3Protocol> ndn = m_nodePtr->GetObject<L3Protocol> ();
  for (uint32_t faceId = 0; faceId < ndn->GetNFaces (); faceId++)
    {
      ndn->GetFace (faceId)->TraceConnectWithoutContext ("JumpDistanceData", MakeCallback (&CarRelayTracer::JumpDistance, this));
    }
}

void
CarRelayTracer::ConnectTx ()
{
  Ptr<L3Protocol> ndn = m_nodePtr->GetObject<L3Protocol> ();
  for (uint32_t faceId = 0; faceId < ndn->GetNFaces (); faceId++)
    {
      ndn->GetFace (faceId)->TraceConnectWithoutContext ("TxData", MakeCallback (&CarRelayTracer::Tx, this));
    }
}

void
CarRelayTracer::ConnectInCache ()
{
  m_nodePtr->GetObject<ContentStore> ()->TraceConnectWithoutContext ("DidAddEntry", MakeCallback (&CarRelayTracer::InCache, this));
}


void
CarRelayTracer::DistanceVsWaiting (double distance, double waiting)
{
  *m_os << Simulator::Now ().ToDouble (Time::S) << "\t" << distance << "\t" << waiting << "\n";
}

void
CarRelayTracer::JumpDistance (Ptr<const Node> node, double jumpDistance)
{
  static int s_jumpDistanceLastNode = -1;
  if (static_cast<int32_t> (node->GetId ()) > s_jumpDistanceLastNode)
    {
      s_jumpDistanceLastNode = node->GetId ();
      *m_os << Simulator::Now ().ToDouble (Time::S) << "\t" << m_node << "\t" << jumpDistance << "\n";
    }
}

void
CarRelayTracer::Tx (Ptr<Node> node, Ptr<const Packet>, const Vector &pos)
{
  *m_os << Simulator::Now ().ToDouble (Time::S) << "\t" << m_node << "\t" << pos.x << "\t" << pos.y << "\t" << pos.z << "\n";
}

void
CarRelayTracer::InCache (Ptr<const ndn::cs::Entry> entry)
{
  Vector pos = m_nodePtr->GetObject<MobilityModel> ()->GetPosition ();
  *m_os << Simulator::Now ().ToDouble (Time::S) << "\t" << m_node << "\t" << pos.x << "\t" << pos.y << "\t" << pos.z << "\n";
}

} // namespace ndn
} // namespace ns3
