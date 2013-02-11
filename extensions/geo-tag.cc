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

#include "geo-tag.h"

namespace ns3
{

TypeId
GeoTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::GeoTag")
    .SetParent<Tag> ()
  ;
  return tid;
}

TypeId
GeoTransmissionTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::GeoTransmissionTag")
    .SetParent<GeoTag> ()
  ;
  return tid;
}

TypeId
GeoSrcTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::GeoSrcTag")
    .SetParent<GeoTag> ()
  ;
  return tid;
}

GeoTag::GeoTag ()
{
}

void
GeoTag::SetPosition (const Vector &position)
{
  m_position = position;
}

const Vector &
GeoTag::GetPosition () const
{
  return m_position;
}

uint32_t
GeoTag::GetSerializedSize() const
{
  return 2 * sizeof (double);
}

void
GeoTag::Serialize(TagBuffer i) const
{
  i.WriteDouble (m_position.x);
  i.WriteDouble (m_position.y);
  // i.WriteDouble (m_position.z);
}

void
GeoTag::Deserialize(ns3::TagBuffer i)
{
  m_position.x = i.ReadDouble ();
  m_position.y = i.ReadDouble ();
  // m_position.z = i.ReadDouble ();
}

void
GeoTag::Print(std::ostream &os) const
{
  os << m_position;
}


} // namespace ns3
