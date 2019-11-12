// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#pragma once


#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QCheckBox>

#include "GxyRenderWindow.hpp"

#include "GxyModel.hpp"

#include <nodes/Connection>
#include <nodes/Node>
#include <nodes/NodeDataModel>

using QtNodes::Connection;
using QtNodes::Node;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "GxyVis.hpp"

#include "Camera.hpp"
#include "CameraDialog.hpp"

#include "Lights.hpp"
#include "LightsDialog.hpp"

class RenderModel : public GxyModel
{
  Q_OBJECT

public:
  RenderModel();

  virtual
  ~RenderModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

  std::shared_ptr<NodeData> outData(PortIndex port) override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("Render"); }

  QString name() const override { return QStringLiteral("Render"); }

  QJsonObject save() const override;
  void restore(QJsonObject const &p) override;

signals:

  void visUpdated(std::shared_ptr<GxyVis>);
  void visDeleted(std::string);
  void cameraChanged(Camera&);
  void lightingChanged(LightingEnvironment&);

private Q_SLOTS:

  void inputConnectionDeleted(QtNodes::Connection const& c) override
  {
    // std::cerr << "inputConnectionDeleted!\n";
    Node *outNode = c.getNode(PortType::Out);
    GxyModel *outModel = (GxyModel *)outNode->nodeDataModel();
    // std::cerr << "to " << outModel->getModelIdentifier() << "\n";
    emit visDeleted(outModel->getModelIdentifier());
    NodeDataModel::inputConnectionDeleted(c);
  }

  void openCameraDialog() 
  {
    CameraDialog *cameraDialog = new CameraDialog(camera);
    cameraDialog->exec();
    cameraDialog->get_camera(camera);
    Q_EMIT cameraChanged(camera);
    delete cameraDialog;
  }

  void openLightsDialog() 
  {
    LightsDialog *lightsDialog = new LightsDialog(lighting);
    lightsDialog->exec();
    lightsDialog->get_lights(lighting);
    Q_EMIT lightingChanged(lighting);
    delete lightsDialog;
  }

private:

  Camera camera;
  LightingEnvironment lighting;

  std::shared_ptr<GxyVis> input;

  GxyRenderWindow renderWindow;
};
