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

#ifndef CAR_RELAY_TRACER_H
#define CAR_RELAY_TRACER_H

#include <boost/shared_ptr.hpp>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/ndn-content-store.h>
#include <ns3/vector.h>

#include <iostream>

namespace ns3 {
namespace ndn {

class CarRelayTracer
{
public:
  enum
    {
      DISTANCE_WAITING = 1,
      JUMP_DISTANCE = 2,
      TX = 4,
      IN_CACHE = 8
    };

  /**
   * @brief Helper method to install tracers on all simulation nodes
   *
   * @param file File to which traces will be written
   * @param types any ORed combination of DISTANCE_WAITING, JUMP_DISTANCE, TX, and IN_CACHE
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<CarRelayTracer> > >
  InstallAll (const std::string &file, int types);


  /**
   * @brief Trace constructor that attaches to the node using node pointer
   * @param os    reference to the output stream
   * @param node  pointer to the node
   */
  CarRelayTracer (boost::shared_ptr<std::ostream> os, Ptr<Node> node);

  void
  ConnectDistance ();

  void
  ConnectJumpDistance ();

  void
  ConnectTx ();

  void
  ConnectInCache ();

private:
  void
  DistanceVsWaiting (double distance, double waiting);

  void
  JumpDistance (Ptr<const Node> node, double jumpDistance);

  void
  Tx (Ptr<Node> node, Ptr<const Packet>, const Vector &pos);

  void InCache (Ptr<const ndn::cs::Entry> entry);

private:
  std::string m_node;
  Ptr<Node> m_nodePtr;

  boost::shared_ptr<std::ostream> m_os;
};

} // namespace ndn
} // namespace ns3

#endif // CAR_RELAY_TRACER_H
