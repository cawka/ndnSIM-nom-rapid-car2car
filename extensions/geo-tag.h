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
 * Authors: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *
 */

#ifndef GEO_TAG_H
#define GEO_TAG_H

#include "ns3/tag.h"
#include "ns3/vector.h"

namespace ns3
{

/**
 * \ingroup Ndn
 * \brief Tag store geo position
 */
class GeoTag : public Tag
{
public:
  static TypeId
  GetTypeId ();

  /**
   * \brief Constructor
   */
  GeoTag ();

  /**
   * \brief Set name for the tag
   */
  void
  SetPosition (const Vector &position);

  /**
   * \brief Get name of the tag
   */
  const Vector &
  GetPosition () const;

  // from Tag
  virtual uint32_t
  GetSerializedSize() const;

  virtual void
  Serialize(TagBuffer i) const;

  virtual void
  Deserialize(TagBuffer i);

  virtual void
  Print(std::ostream&) const;

private:
  Vector m_position;
};

/**
 * \ingroup Ndn
 * @brief Tag to mark last hop transmission
 */
class GeoTransmissionTag : public GeoTag
{
public:
  static TypeId
  GetTypeId ();

  virtual
  TypeId GetInstanceTypeId () const
  {
    return GeoTransmissionTag::GetTypeId ();
  }
};

/**
 * \ingroup Ndn
 * @brief Tag to mark originator
 */
class GeoSrcTag : public GeoTag
{
public:
  static TypeId
  GetTypeId ();

  virtual
  TypeId GetInstanceTypeId () const
  {
    return GeoSrcTag::GetTypeId ();
  }
};

} // namespace ns3

#endif // GEO_TAG_H
