//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "UsdTransform3dBase.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dBase::UsdTransform3dBase(const UsdSceneItem::Ptr& item)
    : UsdTransform3dReadImpl(item)
    , Transform3d()
{
}

const Ufe::Path& UsdTransform3dBase::path() const { return UsdTransform3dReadImpl::path(); }

Ufe::SceneItem::Ptr UsdTransform3dBase::sceneItem() const
{
    return UsdTransform3dReadImpl::sceneItem();
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dBase::translateCmd(double /* x */, double /* y */, double /* z */)
{
    return nullptr;
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dBase::rotateCmd(
    double /* x */,
    double /* y */,
    double /* z */
)
{
    return nullptr;
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dBase::scaleCmd(
    double /* x */,
    double /* y */,
    double /* z */
)
{
    return nullptr;
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::rotatePivotCmd(double x, double y, double z)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::rotatePivot() const
{
    // Called by Maya during transform editing.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::scalePivotCmd(double x, double y, double z)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::scalePivot() const
{
    // Called by Maya during transform editing.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dBase::translateRotatePivotCmd(double, double, double)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::rotatePivotTranslation() const { return Ufe::Vector3d(0, 0, 0); }

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dBase::translateScalePivotCmd(double, double, double)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::scalePivotTranslation() const { return Ufe::Vector3d(0, 0, 0); }

Ufe::SetMatrix4dUndoableCommand::Ptr UsdTransform3dBase::setMatrixCmd(const Ufe::Matrix4d& m)
{
    return nullptr;
}

Ufe::Matrix4d UsdTransform3dBase::matrix() const { return UsdTransform3dReadImpl::matrix(); }

Ufe::Matrix4d UsdTransform3dBase::segmentInclusiveMatrix() const
{
    return UsdTransform3dReadImpl::segmentInclusiveMatrix();
}

Ufe::Matrix4d UsdTransform3dBase::segmentExclusiveMatrix() const
{
    return UsdTransform3dReadImpl::segmentExclusiveMatrix();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
