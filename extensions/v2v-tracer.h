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
 *
 */

#ifndef V2V_TRACER_H
#define V2V_TRACER_H

#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

#include <ns3/ndn-content-store.h>
#include <ns3/ndn-face.h>
#include <ns3/packet.h>
#include <ns3/node.h>

namespace ns3 {
namespace ndn {

class V2vTracer
{
public:
  /**
   * @brief Helper method to install tracers on all simulation nodes
   *
   * @param file File to which traces will be written
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static boost::tuple< boost::shared_ptr<std::ostream>, std::list<boost::shared_ptr<V2vTracer> > >
  InstallAll (const std::string &file);

  /**
   * @brief Trace constructor that attaches to the node using node pointer
   * @param os    reference to the output stream
   * @param node  pointer to the node
   */
  V2vTracer (boost::shared_ptr<std::ostream> os, Ptr<Node> node);

  /**
   * @brief Connect traces on a node
   */
  void
  Connect ();


private:
  void
  PrintHeader (std::ostream &os) const;

  void DidAddEntry (Ptr<const cs::Entry> csEntry);

  void InInterest (Ptr<const Interest> header, Ptr<const Face> face);

  void PhyOutData (Ptr<const Packet> packet);

  void Canceling (Ptr<Node> node, Ptr<const Packet> packet);

private:
  std::string m_node;
  Ptr<Node> m_nodePtr;

  boost::shared_ptr<std::ostream> m_os;
};

} // namespace ndn
} // namespace ns3

#endif // V2V_TRACER_H
