//
// Copyright 2023 Autodesk
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

#include "UsdUndoToggleInstanceableCommand.h"

namespace USDUFE_NS_DEF {

UsdUndoToggleInstanceableCommand::UsdUndoToggleInstanceableCommand(const PXR_NS::UsdPrim& prim)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
{
}

void UsdUndoToggleInstanceableCommand::executeImplementation()
{
    if (!_stage)
        return;

    PXR_NS::UsdPrim prim = _stage->GetPrimAtPath(_primPath);
    if (!prim.IsValid())
        return;

    prim.SetInstanceable(!prim.IsInstanceable());
}

} // namespace USDUFE_NS_DEF
