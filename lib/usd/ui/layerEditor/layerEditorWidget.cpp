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

#include "layerEditorWidget.h"

#include "abstractCommandHook.h"
#include "dirtyLayersCountBadge.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "qtUtils.h"
#include "stageSelectorWidget.h"
#include "stringResources.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

#include <maya/MGlobal.h>

#include <QtCore/QItemSelectionModel>
#include <QtCore/QTimer>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QActionGroup>
#else
#include <QtWidgets/QActionGroup>
#endif

#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <cstddef>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using namespace UsdLayerEditor;

// create the default menus on the parent QMainWindow
void setupDefaultMenu(SessionState* in_sessionState, QMainWindow* in_parent)
{
    auto menuBar = in_parent->menuBar();
    // don't add menu twice -- this window is destroyed and re-created on new scene
    if (menuBar->actions().length() == 0) {
        auto createMenu = menuBar->addMenu(StringResources::getAsQString(StringResources::kCreate));
        auto aboutToShowCallback = [createMenu, in_sessionState]() {
            if (createMenu->actions().length() == 0) {
                in_sessionState->setupCreateMenu(createMenu);
            }
        };
        // we delay populating the create menu to first show, because in the python prototype, if
        // the layer editor was docked, the menu would get populated before the runtime commands had
        // the time to be created
        QObject::connect(createMenu, &QMenu::aboutToShow, in_parent, aboutToShowCallback);

        auto optionMenu = menuBar->addMenu(StringResources::getAsQString(StringResources::kOption));
        auto action = optionMenu->addAction(
            StringResources::getAsQString(StringResources::kAutoHideSessionLayer));
        QObject::connect(
            action, &QAction::toggled, in_sessionState, &SessionState::setAutoHideSessionLayer);
        action->setCheckable(true);
        action->setChecked(in_sessionState->autoHideSessionLayer());

        auto helpMenu = menuBar->addMenu(StringResources::getAsQString(StringResources::kHelp));
        helpMenu->addAction(
            StringResources::getAsQString(StringResources::kHelpOnUSDLayerEditor),
            [in_sessionState]() { in_sessionState->commandHook()->showLayerEditorHelp(); });
    }
}

} // namespace

///////////////////////////////////////////////////////////////////////////////

namespace UsdLayerEditor {
LayerEditorWidget::LayerEditorWidget(SessionState& in_sessionState, QMainWindow* in_parent)
    : QWidget(in_parent)
    , _sessionState(in_sessionState)
    , _saveButtonParent(nullptr)
{
    setupLayout();
    ::setupDefaultMenu(&in_sessionState, in_parent);
}

// helper for setupLayout
QLayout* LayerEditorWidget::setupLayout_toolbar()
{
    auto buttonSize = DPIScale(24);
    auto margin = DPIScale(12);
    auto toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(margin, 0, 0, 0);
    auto buttonAlignment = Qt::AlignLeft | Qt::AlignRight;

    auto addHIGButton
        = [buttonSize, toolbar, buttonAlignment](const QString& iconName, const QString& tooltip) {
              auto higButtonYOffset = DPIScale(4);
              auto higBtn = new QPushButton();
              higBtn->move(0, higButtonYOffset);
              QtUtils::setupButtonWithHIGBitmaps(higBtn, iconName);
              higBtn->setFixedSize(buttonSize, buttonSize);
              higBtn->setToolTip(tooltip);
              toolbar->addWidget(higBtn, 0, buttonAlignment);
              return higBtn;
          };

    _buttons._newLayer = addHIGButton(
        ":/UsdLayerEditor/LE_add_layer",
        StringResources::getAsQString(StringResources::kAddNewLayer));
    // clicked callback
    connect(
        _buttons._newLayer,
        &QAbstractButton::clicked,
        this,
        &LayerEditorWidget::onNewLayerButtonClicked);
    // update layer button on stage change
    connect(
        _treeView->model(),
        &QAbstractItemModel::modelReset,
        this,
        &LayerEditorWidget::updateNewLayerButton);
    // update layer button if muted state changes
    connect(
        _treeView->model(),
        &QAbstractItemModel::dataChanged,
        this,
        &LayerEditorWidget::updateNewLayerButton);
    // update layer button on selection change
    connect(
        _treeView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &LayerEditorWidget::updateNewLayerButton);

    _buttons._loadLayer = addHIGButton(
        ":/UsdLayerEditor/LE_import_layer",
        StringResources::getAsQString(StringResources::kLoadExistingLayer));
    // clicked callback
    connect(
        _buttons._loadLayer,
        &QAbstractButton::clicked,
        this,
        &LayerEditorWidget::onLoadLayersButtonClicked);

    toolbar->addStretch();

    { // save stage button: contains a push button and a "badge" widget
        _saveButtonParent = new QWidget();
        auto saveButtonYOffset = DPIScale(4);
        auto saveButtonSize = QSize(buttonSize + DPIScale(12), buttonSize + saveButtonYOffset);
        _saveButtonParent->setFixedSize(saveButtonSize);
        auto saveStageBtn = new QPushButton(_saveButtonParent);
        saveStageBtn->move(0, saveButtonYOffset);
        QtUtils::setupButtonWithHIGBitmaps(saveStageBtn, ":/UsdLayerEditor/LE_save_all");
        saveStageBtn->setFixedSize(buttonSize, buttonSize);
        saveStageBtn->setToolTip(
            StringResources::getAsQString(StringResources::kSaveAllEditsInLayerStack));
        connect(
            saveStageBtn,
            &QAbstractButton::clicked,
            this,
            &LayerEditorWidget::onSaveStageButtonClicked);

        auto dirtyCountBadge = new DirtyLayersCountBadge(_saveButtonParent);
        dirtyCountBadge->setFixedSize(saveButtonSize);
        toolbar->addWidget(_saveButtonParent, 0, buttonAlignment);

        _buttons._saveStageButton = saveStageBtn;
        _buttons._dirtyCountBadge = dirtyCountBadge;
    }

    // update buttons on stage change for example dirty count
    connect(
        _treeView->model(),
        &QAbstractItemModel::modelReset,
        this,
        &LayerEditorWidget::updateButtonsOnIdle);
    // update dirty count on dirty notfication
    connect(
        _treeView->model(),
        &QAbstractItemModel::dataChanged,
        this,
        &LayerEditorWidget::updateButtonsOnIdle);

    return toolbar;
}

void LayerEditorWidget::setupLayout()
{
    _treeView = new LayerTreeView(&_sessionState, this);

    auto mainVLayout = new QVBoxLayout();
    mainVLayout->setSpacing(DPIScale(4));

    auto stageSelector = new StageSelectorWidget(&_sessionState, this);
    mainVLayout->addWidget(stageSelector);

    auto toolbarLayout = setupLayout_toolbar();
    mainVLayout->addLayout(toolbarLayout);

    mainVLayout->addWidget(_treeView);

    setLayout(mainVLayout);

    updateNewLayerButton();
    updateButtons();
}

void LayerEditorWidget::updateButtonsOnIdle()
{
    if (!_updateButtonsOnIdle) {
        _updateButtonsOnIdle = true;
        QTimer::singleShot(0, this, &LayerEditorWidget::updateButtons);
    }
}

void LayerEditorWidget::updateNewLayerButton()
{
    bool disabled = _treeView->model()->rowCount() == 0;
    if (!disabled) {
        auto selectionModel = _treeView->selectionModel();
        // enabled if no or one layer selected
        disabled = selectionModel->selectedRows().size() > 1;

        // also disable if the selected layer is muted or invalid
        if (!disabled) {
            auto item = _treeView->currentLayerItem();
            if (item) {
                disabled = item->isInvalidLayer() || item->appearsMuted() || item->isReadOnly();
            }
        }
    }
    _buttons._newLayer->setDisabled(disabled);
    _buttons._loadLayer->setDisabled(disabled);
}

void LayerEditorWidget::updateButtons()
{
    if (_sessionState.commandHook()->isProxyShapeSharedStage(
            _sessionState.stageEntry()._proxyShapePath)) {
        _saveButtonParent->setVisible(true);
        const auto layers = _treeView->layerTreeModel()->getAllNeedsSavingLayers();
        int        count = static_cast<int>(layers.size());
        _buttons._dirtyCountBadge->updateCount(count);
        bool disable = count == 0;
        QtUtils::disableHIGButton(_buttons._saveStageButton, disable);
    } else {
        _saveButtonParent->setVisible(false);
    }
    _updateButtonsOnIdle = false;
}

void LayerEditorWidget::onNewLayerButtonClicked()
{
    // if nothing or root selected, add new layer to top of root
    auto model = _treeView->layerTreeModel();
    auto selectionModel = _treeView->selectionModel();
    auto selection = selectionModel->selectedRows();
    bool addToRoot = false;

    LayerTreeItem* layerTreeItem;
    if (selection.size() == 0) {
        addToRoot = true;
        layerTreeItem = model->layerItemFromIndex(model->rootLayerIndex());
    } else {
        layerTreeItem = model->layerItemFromIndex(selection[0]);
        // this test catches both root layer and session layer
        if (layerTreeItem->parent() == nullptr) {
            addToRoot = true;
        }
    }

    if (addToRoot) {
        layerTreeItem->addAnonymousSublayer();
    } else {
        // add a sibling to the selection
        UndoContext context(_sessionState.commandHook(), "Add Anonymous Layer");
        auto        parentItem = layerTreeItem->parentLayerItem();
        int         rowToInsert = layerTreeItem->row();
        auto        newLayer = parentItem->addAnonymousSublayerAndReturn();
        // move it to the right place, if it's not top
        if (rowToInsert > 0) {
            context.hook()->removeSubLayerPath(parentItem->layer(), newLayer->GetIdentifier());
            context.hook()->insertSubLayerPath(
                parentItem->layer(), newLayer->GetIdentifier(), rowToInsert);
            model->selectUsdLayerOnIdle(newLayer);
        }
    }
}

void LayerEditorWidget::onLoadLayersButtonClicked()
{
    auto           model = _treeView->layerTreeModel();
    auto           selectionModel = _treeView->selectionModel();
    auto           selection = selectionModel->selectedRows();
    LayerTreeItem* layerTreeItem;
    if (selection.size() == 0) {
        layerTreeItem = model->layerItemFromIndex(model->rootLayerIndex());
    } else {
        layerTreeItem = model->layerItemFromIndex(selection[0]);
    }
    layerTreeItem->loadSubLayers(this);
}

void LayerEditorWidget::onSaveStageButtonClicked() { _treeView->layerTreeModel()->saveStage(this); }

} // namespace UsdLayerEditor
