// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "aliceVision/numeric/numeric.hpp"
#include "aliceVision/camera/cameraCommon.hpp"

#include <vector>

namespace aliceVision {
namespace camera {

/**
 * Implement a simple Fish-eye camera model
 *
 * This is an adaptation of the Fisheye distortion model implemented in OpenCV:
 * https://github.com/Itseez/opencv/blob/master/modules/calib3d/src/fisheye.cpp
 */
class PinholeFisheye : public Pinhole
{
  protected:
  // center of distortion is applied by the Intrinsics class
  std::vector<double> _distortionParams; // K1, K2, K3, K4


  public:

  PinholeFisheye(
    int w = 0, int h = 0,
    double focal = 0.0, double ppx = 0, double ppy = 0,
    double k1 = 0.0, double k2 = 0.0, double k3 = 0.0, double k4 = 0.0)
        :Pinhole(w, h, focal, ppx, ppy)
  {
    _distortionParams = {k1, k2, k3, k4};
  }

  PinholeFisheye* clone() const { return new PinholeFisheye(*this); }
  void assign(const IntrinsicBase& other) { *this = dynamic_cast<const PinholeFisheye&>(other); }

  EINTRINSIC getType() const { return PINHOLE_CAMERA_FISHEYE; }

  virtual bool have_disto() const { return true;}

  virtual Vec2 add_disto(const Vec2 & p) const
  {
    const double eps = 1e-8;
    const double k1 = _distortionParams[0], k2 = _distortionParams[1], k3 = _distortionParams[2], k4 = _distortionParams[3];
    const double r = std::hypot(p(0), p(1));
    const double theta = std::atan(r);
    const double
      theta2 = theta*theta,
      theta3 = theta2*theta,
      theta4 = theta2*theta2,
      theta5 = theta4*theta,
      theta6 = theta3*theta3,
      theta7 = theta6*theta,
      theta8 = theta4*theta4,
      theta9 = theta8*theta;
    const double theta_dist = theta + k1*theta3 + k2*theta5 + k3*theta7 + k4*theta9;
    const double inv_r = r > eps ? 1.0/r : 1.0;
    const double cdist = r > eps ? theta_dist * inv_r : 1.0;
    return  p*cdist;
  }

  virtual Vec2 remove_disto(const Vec2 & p) const
  {
    const double eps = 1e-8;
    double scale = 1.0;
    const double theta_dist = std::hypot(p[0], p[1]);
    if (theta_dist > eps)
    {
      double theta = theta_dist;
      for (int j = 0; j < 10; ++j)
      {
        const double
          theta2 = theta*theta,
          theta4 = theta2*theta2,
          theta6 = theta4*theta2,
          theta8 = theta6*theta2;
        theta = theta_dist /
          (1 + _distortionParams[0] * theta2
             + _distortionParams[1] * theta4
             + _distortionParams[2] * theta6
             + _distortionParams[3] * theta8);
      }
      scale = std::tan(theta) / theta_dist;
    }
    return p * scale;
  }

  // Data wrapper for non linear optimization (get data)
  virtual std::vector<double> getParams() const
  {
    std::vector<double> params = Pinhole::getParams();
    params.push_back(_distortionParams[0]);
    params.push_back(_distortionParams[1]);
    params.push_back(_distortionParams[2]);
    params.push_back(_distortionParams[3]);
    return params;
  }

  virtual std::vector<double> getDistortionParams() const
  {
    return _distortionParams;
  }

  // Data wrapper for non linear optimization (update from data)
  virtual bool updateFromParams(const std::vector<double> & params)
  {
    if (params.size() == 7)
    {
      this->setK(params[0], params[1], params[2]);
      _distortionParams = {params[3], params[4], params[5], params[6]};
      return true;
    }
    return false;
  }

  /// Return the un-distorted pixel (with removed distortion)
  virtual Vec2 get_ud_pixel(const Vec2& p) const
  {
    return cam2ima( remove_disto(ima2cam(p)) );
  }

  /// Return the distorted pixel (with added distortion)
  virtual Vec2 get_d_pixel(const Vec2& p) const
  {
    return cam2ima( add_disto(ima2cam(p)) );
  }

  // Serialization
  template <class Archive>
  void save( Archive & ar) const
  {
    Pinhole::save(ar);
    ar(cereal::make_nvp("fisheye4", _distortionParams));
  }

  // Serialization
  template <class Archive>
  void load( Archive & ar)
  {
    Pinhole::load(ar);
    ar(cereal::make_nvp("fisheye4", _distortionParams));
  }
};


} // namespace camera
} // namespace aliceVision

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>

CEREAL_REGISTER_TYPE_WITH_NAME(aliceVision::camera::PinholeFisheye, "fisheye4");