/*==============================================================================

  Copyright (c) The Intervention Centre
  Oslo University Hospital, Oslo, Norway. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Rafael Palomar (The Intervention Centre,
  Oslo University Hospital) and was supported by The Research Council of Norway
  through the ALive project (grant nr. 311393).

==============================================================================*/

#include "vtkMRMLMarkupsShapeNode.h"
#include "vtkMRMLMeasurementShape.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMRMLSliceNode.h>
#include <vtkCollection.h>
#include <vtkCallbackCommand.h>

//--------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsShapeNode);

//--------------------------------------------------------------------------------
vtkMRMLMarkupsShapeNode::vtkMRMLMarkupsShapeNode()
{
  this->SetShapeName(Sphere);
  
  this->OnPointPositionUndefinedCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->OnPointPositionUndefinedCallback->SetClientData( reinterpret_cast<void *>(this) );
  this->OnPointPositionUndefinedCallback->SetCallback( vtkMRMLMarkupsShapeNode::OnPointPositionUndefined );
  this->AddObserver(vtkMRMLMarkupsNode::PointPositionUndefinedEvent, this->OnPointPositionUndefinedCallback);
}

//--------------------------------------------------------------------------------
vtkMRMLMarkupsShapeNode::~vtkMRMLMarkupsShapeNode()
{
  this->RemoveObserver(this->OnPointPositionUndefinedCallback);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::SetShapeName(int shapeName)
{
  this->ShapeName = shapeName;
  switch (shapeName)
  {
    case Sphere :
      this->RequiredNumberOfControlPoints = 2;
      this->MaximumNumberOfControlPoints = 2;
      this->ForceSphereMeasurements();
      break;
    case Ring:
      // Third point is used to calculate normal relative to the center in 3D view.
      this->RequiredNumberOfControlPoints = 3;
      this->MaximumNumberOfControlPoints = 3;
      this->ForceRingMeasurements();
      break;
    case Disk:
      // Point 0 : always the center.
      this->RequiredNumberOfControlPoints = 3;
      this->MaximumNumberOfControlPoints = 3;
      this->ForceDiskMeasurements();
      break;
    case Tube:
      /*
       * RequiredNumberOfControlPoints should be 4, but toolbar new control point button remains grayed for ever.
       * With -1, it still remains grayed, but hovering on a control point activates the new control point button.
       * 
       * N.B. - control points need not and are not required to be on the surface.
       * A control point pair merely defines a radius value and a middle point for the spline (centerline).
       * In practice, we would place points on the walls of a diseased artery, a short part of it.
       * All this is useless for healthy arteries : we have real structures using segmentation,
       * that do not have perfectly circular cross-sections.
       */
      this->RequiredNumberOfControlPoints = -1;
      this->MaximumNumberOfControlPoints = -1;
      this->ForceTubeMeasurements();
    default :
      vtkErrorMacro("Unknown shape.");
      return;
  };
  if (this->MaximumNumberOfControlPoints > 0)
  {
    while (this->GetNumberOfControlPoints() > this->MaximumNumberOfControlPoints)
    {
      this->RemoveNthControlPoint(this->MaximumNumberOfControlPoints);
    }
  }
  else
  {
    this->RemoveAllControlPoints();
  }
  this->Modified();
}

//---------------------- For disk shape -------------------------------------
bool vtkMRMLMarkupsShapeNode::DescribeDiskPointSpacing(double * closestPoint, double * farthestPoint,
                                                     double& innerRadius, double& outerRadius)
{
  if (this->ShapeName != Disk)
  {
    vtkErrorMacro("Current shape is not a disk.");
    return false;
  }
  if (this->GetNumberOfDefinedControlPoints(true) != 3)
  {
    return false;
  }
  double p1[3] = { 0.0 }; // center
  double p2[3] = { 0.0 };
  double p3[3] = { 0.0 };
  this->GetNthControlPointPositionWorld(0, p1);
  this->GetNthControlPointPositionWorld(1, p2);
  this->GetNthControlPointPositionWorld(2, p3);
  
  double distance2 = std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  double distance3 = std::sqrt(vtkMath::Distance2BetweenPoints(p1, p3));
  
  if (distance2 <= distance3)
  {
    closestPoint[0] = p2[0];
    closestPoint[1] = p2[1];
    closestPoint[2] = p2[2];
    farthestPoint[0] = p3[0];
    farthestPoint[1] = p3[1];
    farthestPoint[2] = p3[2];
    innerRadius = distance2;
    outerRadius = distance3;
  }
  else
  {
    closestPoint[0] = p3[0];
    closestPoint[1] = p3[1];
    closestPoint[2] = p3[2];
    farthestPoint[0] = p2[0];
    farthestPoint[1] = p2[1];
    farthestPoint[2] = p2[2];
    innerRadius = distance3;
    outerRadius = distance2;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::FindLinearCoordinateByDistance(const double * p1, const double * p2,
                                                            double * result, const double difference)
{
  // Relative to p1, itself placed at origin. Simplifies reasoning.
  const double rp2[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
  const double lineLength = std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  const double factor = (1.0 + (difference / lineLength));
  result[0] = p1[0] + (rp2[0] * factor);
  result[1] = p1[1] + (rp2[1] * factor);
  result[2] = p1[2] + (rp2[2] * factor);
}

//----------------------------API only----------------------------------------
void vtkMRMLMarkupsShapeNode::SetRadius(double radius)
{
  if (this->ShapeName == Disk)
  {
    vtkErrorMacro("Current shape is a disk. Use SetInnerRadius and SetOuterRadius");
    return;
  }
  if (radius <= 0)
  {
    vtkErrorMacro("Radius must be greater than zero.");
    return;
  }
  double rasP1[3] = { 0.0 };
  double rasP2[3] = { 0.0 };
  this->GetNthControlPointPositionWorld(0, rasP1);
  this->GetNthControlPointPositionWorld(1, rasP2);
  const double lineLength = std::sqrt(vtkMath::Distance2BetweenPoints(rasP1, rasP2));
  const double currentRadius = (this->RadiusMode == Centered)
                              ? lineLength : lineLength / 2;
  const double difference = radius - currentRadius;
  double rasP2Shifted[3] = { 0.0 };
  this->FindLinearCoordinateByDistance(rasP1, rasP2, rasP2Shifted, difference);
  
  this->SetNthControlPointPositionWorld(1, rasP2Shifted);
  // Don't move center, move p1.
  if (this->RadiusMode == Circumferential)
  {
    double rasP1Shifted[3] = { 0.0 };
    this->FindLinearCoordinateByDistance(rasP2, rasP1, rasP1Shifted, difference);
    this->SetNthControlPointPositionWorld(0, rasP1Shifted);
  }
  // Text actor does not move until mouse is hovered on a control point.
}

// Merging all shapes introduces complexity, API becomes bad looking, with shape specific functions.
//----------------------------API only----------------------------------------
void vtkMRMLMarkupsShapeNode::SetInnerRadius(double radius)
{
  if (this->ShapeName != Disk)
  {
    vtkErrorMacro("Current shape is not a disk. Use SetRadius.");
    return;
  }
  if (radius <= 0)
  {
    vtkErrorMacro("Radius must be greater than zero.");
    return;
  }
  double closestPoint[3] = { 0.0 };
  double farthestPoint[3] = { 0.0 };
  double innerRadius = 0.0, outerRadius = 0.0;
  if (!this->DescribeDiskPointSpacing(closestPoint, farthestPoint, innerRadius, outerRadius))
  {
    vtkDebugMacro("Point proximity description failure.");
    return;
  }
  if (radius >= outerRadius)
  {
    vtkErrorMacro("Inner radius must be less than outer radius.");
    return;
  }
  double rasP1[3] = { 0.0 };
  this->GetNthControlPointPositionWorld(0, rasP1);
  
  const double difference = radius - innerRadius;
  double closestPointShifted[3] = { 0.0 };
  this->FindLinearCoordinateByDistance(rasP1, closestPoint, closestPointShifted, difference);
  
  this->SetNthControlPointPositionWorld(this->GetClosestControlPointIndexToPositionWorld(closestPoint), closestPointShifted);
}

//----------------------------API only----------------------------------------
void vtkMRMLMarkupsShapeNode::SetOuterRadius(double radius)
{
  if (this->ShapeName != Disk)
  {
    vtkErrorMacro("Current shape is not a disk. Use SetRadius.");
    return;
  }
  if (radius <= 0)
  {
    vtkErrorMacro("Radius must be greater than zero.");
    return;
  }
  double closestPoint[3] = { 0.0 };
  double farthestPoint[3] = { 0.0 };
  double innerRadius = 0.0, outerRadius = 0.0;
  if (!this->DescribeDiskPointSpacing(closestPoint, farthestPoint, innerRadius, outerRadius))
  {
    vtkDebugMacro("Point proximity description failure.");
    return;
  }
  if (radius <= innerRadius)
  {
    vtkErrorMacro("Outer radius must be greater than inner radius.");
    return;
  }
  double rasP1[3] = { 0.0 };
  this->GetNthControlPointPositionWorld(0, rasP1);
  
  const double difference = radius - outerRadius;
  double farthestPointShifted[3] = { 0.0 };
  this->FindLinearCoordinateByDistance(rasP1, farthestPoint, farthestPointShifted, difference);
  
  this->SetNthControlPointPositionWorld(this->GetClosestControlPointIndexToPositionWorld(farthestPoint), farthestPointShifted);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ResliceToControlPoints()
{
  switch (this->ShapeName)
  {
    case Sphere :
      this->ResliceToLine();
      break;
    case Ring:
      this->ResliceToPlane();
      break;
    case Disk:
      this->ResliceToPlane();
      break;
    case Tube:
      break;
    default :
      vtkErrorMacro("Unknown shape.");
      return;
  };
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ResliceToPlane()
{
  if (!this->ResliceNode)
  {
    return;
  }
  vtkMRMLSliceNode * resliceNode = vtkMRMLSliceNode::SafeDownCast(this->ResliceNode);
  if (!resliceNode)
  {
    return;
  }
  double rasP1[3] = { 0.0 };
  double rasP2[3] = { 0.0 };
  double rasP3[3] = { 0.0 };
  double rasNormal[3] = { 0.0 };
  this->GetNthControlPointPositionWorld(0, rasP1);
  this->GetNthControlPointPositionWorld(1, rasP2);
  this->GetNthControlPointPositionWorld(2, rasP3);
  
  // Relative to rasP1 (center)
  double rRasP2[3] = { rasP2[0] - rasP1[0], rasP2[1] - rasP1[1], rasP2[2] - rasP1[2] };
  double rRasP3[3] = { rasP3[0] - rasP1[0], rasP3[1] - rasP1[1], rasP3[2] - rasP1[2] };
  
  vtkMath::Cross(rRasP2, rRasP3, rasNormal);
  if (rasNormal[0] == 0.0 && rasNormal[1] == 0.0 && rasNormal[2] == 0.0)
  {
    return;
  }
  resliceNode->SetSliceToRASByNTP(
    rasNormal[0], rasNormal[1], rasNormal[2],
    rasP2[0], rasP2[1], rasP2[2],
    rasP1[0], rasP1[1], rasP1[2],
    0);
  resliceNode->UpdateMatrices();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ResliceToLine()
{
  if (!this->ResliceNode)
  {
    return;
  }
  vtkMRMLSliceNode * resliceNode = vtkMRMLSliceNode::SafeDownCast(this->ResliceNode);
  if (!resliceNode)
  {
    return;
  }
  double rasP1[3] = { 0.0 };
  double rasP2[3] = { 0.0 };
  double rasNormal[3] = { 0.0 };
  this->GetNthControlPointPositionWorld(0, rasP1);
  this->GetNthControlPointPositionWorld(1, rasP2);
  
  vtkMath::Cross(rasP1, rasP2, rasNormal);
  if (rasNormal[0] == 0.0 && rasNormal[1] == 0.0 && rasNormal[2] == 0.0)
  {
    return;
  }
  resliceNode->SetSliceToRASByNTP(
    rasNormal[0], rasNormal[1], rasNormal[2],
    rasP2[0], rasP2[1], rasP2[2],
    rasP1[0], rasP1[1], rasP1[2],
    0);
  resliceNode->UpdateMatrices();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ForceDiskMeasurements()
{
  auto addLengthMeasurement = [&] (const char * name, bool enabled)
  {
    vtkNew<vtkMRMLMeasurementShape> measurement;
    measurement->SetUnits("mm");
    measurement->SetPrintFormat("%-#4.4g%s");
    measurement->SetName(name);
    measurement->SetInputMRMLNode(this);
    measurement->SetEnabled(enabled);
    this->Measurements->AddItem(measurement);
  };
  
  auto addAreaMeasurement = [&](const char * name) {
    vtkNew<vtkMRMLMeasurementShape> measurement;
    measurement->SetUnits("cm2");
    measurement->SetDisplayCoefficient(0.01);
    measurement->SetPrintFormat("%-#4.4g%s");
    measurement->SetName(name);
    measurement->SetInputMRMLNode(this);
    measurement->SetEnabled(false);
    this->Measurements->AddItem(measurement);
  };
  
  this->RemoveAllMeasurements();
  addLengthMeasurement("innerRadius", true);
  addLengthMeasurement("outerRadius", true);
  addLengthMeasurement("width", false);
  addAreaMeasurement("area");
  addAreaMeasurement("innerArea");
  addAreaMeasurement("outerArea");
  /*
   * If we switch from Ring to Disk, inner and outer radii may be identical.
   * The Disk is not drawn, that's OK.
   * But the measurements are surprisingly not updated until an MRML event.
   * Force an update.
   */
  this->UpdateAllMeasurements();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ForceRingMeasurements()
{
  this->RemoveAllMeasurements();
  
  vtkNew<vtkMRMLMeasurementShape> radiusMeasurement;
  radiusMeasurement->SetName("radius");
  radiusMeasurement->SetUnits("mm");
  radiusMeasurement->SetPrintFormat("%-#4.4g%s");
  radiusMeasurement->SetInputMRMLNode(this);
  radiusMeasurement->SetEnabled(true);
  this->Measurements->AddItem(radiusMeasurement);
  
  vtkNew<vtkMRMLMeasurementShape> areaMeasurement;
  areaMeasurement->SetName("area");
  areaMeasurement->SetUnits("cm2");
  areaMeasurement->SetDisplayCoefficient(0.01);
  areaMeasurement->SetPrintFormat("%-#4.4g%s");
  areaMeasurement->SetInputMRMLNode(this);
  areaMeasurement->SetEnabled(false);
  this->Measurements->AddItem(areaMeasurement);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ForceSphereMeasurements()
{
  this->RemoveAllMeasurements();
  
  vtkNew<vtkMRMLMeasurementShape> radiusMeasurement;
  radiusMeasurement->SetName("radius");
  radiusMeasurement->SetUnits("mm");
  radiusMeasurement->SetPrintFormat("%-#4.4g%s");
  radiusMeasurement->SetInputMRMLNode(this);
  radiusMeasurement->SetEnabled(true);
  this->Measurements->AddItem(radiusMeasurement);
  
  vtkNew<vtkMRMLMeasurementShape> areaMeasurement;
  areaMeasurement->SetName("area");
  areaMeasurement->SetUnits("cm2");
  areaMeasurement->SetDisplayCoefficient(0.01);
  areaMeasurement->SetPrintFormat("%-#4.4g%s");
  areaMeasurement->SetInputMRMLNode(this);
  areaMeasurement->SetEnabled(false);
  this->Measurements->AddItem(areaMeasurement);
  
  vtkNew<vtkMRMLMeasurementShape> volumeMeasurement;
  volumeMeasurement->SetName("volume");
  volumeMeasurement->SetUnits("cm3");
  volumeMeasurement->SetDisplayCoefficient(0.01);
  volumeMeasurement->SetPrintFormat("%-#4.4g%s");
  volumeMeasurement->SetInputMRMLNode(this);
  volumeMeasurement->SetEnabled(false);
  this->Measurements->AddItem(volumeMeasurement);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::ForceTubeMeasurements()
{
  this->RemoveAllMeasurements();
  
  vtkNew<vtkMRMLMeasurementShape> areaMeasurement;
  areaMeasurement->SetName("area");
  areaMeasurement->SetUnits("cm2");
  areaMeasurement->SetDisplayCoefficient(0.01);
  areaMeasurement->SetPrintFormat("%-#4.4g%s");
  areaMeasurement->SetInputMRMLNode(this);
  areaMeasurement->SetEnabled(false);
  this->Measurements->AddItem(areaMeasurement);
  
  vtkNew<vtkMRMLMeasurementShape> volumeMeasurement;
  volumeMeasurement->SetName("volume");
  volumeMeasurement->SetUnits("cm3");
  volumeMeasurement->SetDisplayCoefficient(0.01);
  volumeMeasurement->SetPrintFormat("%-#4.4g%s");
  volumeMeasurement->SetInputMRMLNode(this);
  volumeMeasurement->SetEnabled(true);
  this->Measurements->AddItem(volumeMeasurement);
}

//----------------------------------------------------------------------------
/*
 * Tube : remove an adjacent point.
 * Toggling a point status in the markups module complicates things, don't react here.
 */
void vtkMRMLMarkupsShapeNode::OnPointPositionUndefined(vtkObject* caller, unsigned long event, void* clientData, void* callData)
{
  vtkMRMLMarkupsShapeNode * client = reinterpret_cast<vtkMRMLMarkupsShapeNode*>(clientData);
  if (!client || client->GetShapeName() != Tube
      || client->GetNumberOfUndefinedControlPoints() > 0)
  {
    return;
  }
  
  if (client->RemovingPairControlPoint || client->GetNumberOfControlPoints() == 0)
  {
    // Point removal was triggered by this function, not in UI.
    client->RemovingPairControlPoint = false;
    return;
  }
  
  const int removedIndex = *(static_cast<int*> (callData));
  if ((removedIndex % 2) == 0 )
  {
    // Already removed point in UI is not last point of an uneven number of points.
    if (client->GetNumberOfControlPoints() > removedIndex)
    {
      client->RemovingPairControlPoint = true;
      client->RemoveNthControlPoint(removedIndex);
    }
    else
    {
      // If the last point of an uneven number of points is removed in UI, there is no next point to remove.
      client->RemovingPairControlPoint = false;
    }
  }
  else 
  {
    client->RemovingPairControlPoint = true;
    client->RemoveNthControlPoint(removedIndex - 1);
  }
}

//----------------------------------------------------------------------------
double vtkMRMLMarkupsShapeNode::GetRadiusAtNthControlPoint(int n)
{
  if (this->GetShapeName() != Tube)
  {
    vtkErrorMacro("Not a Tube shape.");
    return -1.0;
  }
  if (n < 0
    || this->GetNumberOfUndefinedControlPoints() > 0
    || this->GetNumberOfDefinedControlPoints() < 4
    || (this->GetNumberOfDefinedControlPoints() % 2) != 0
    || n >= this->GetNumberOfDefinedControlPoints())
  {
    vtkErrorMacro("Tube shape has undefined control points, or odd number of control points,"
                  " or less than 4 control points, or fewer control points than requested.");
    return -1.0;
  }
  double radius = 0.0;
  double p1[3] = { 0.0 };
  double p2[3] = { 0.0 };
  if ((n % 2) == 0)
  {
    this->GetNthControlPointPositionWorld(n, p1);
    this->GetNthControlPointPositionWorld(n + 1, p2);
  }
  else {
    this->GetNthControlPointPositionWorld(n, p2);
    this->GetNthControlPointPositionWorld(n - 1, p1);
  }
  radius = (std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2))) / 2.0;
  return radius;
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsShapeNode::SetRadiusAtNthControlPoint(int n, double radius)
{
  if (radius <= 0.0)
  {
    vtkErrorMacro("Requested radius must be greater than 0.0.");
    return;
  }
  double currentRadius = this->GetRadiusAtNthControlPoint(n);
  if (currentRadius <= 0)
  {
    return;
  }
  
  double p1[3] = { 0.0 };
  double p2[3] = { 0.0 };
  if ((n % 2) == 0)
  {
    this->GetNthControlPointPositionWorld(n, p1);
    this->GetNthControlPointPositionWorld(n + 1, p2);
  }
  else {
    this->GetNthControlPointPositionWorld(n, p2);
    this->GetNthControlPointPositionWorld(n - 1, p1);
  }
  const double middlePoint[3] = { (p1[0] + p2[0]) / 2.0,
                            (p1[1] + p2[1]) / 2.0,
                            (p1[2] + p2[2]) / 2.0};
  double p1New[3] = { 0.0 };
  double p2New[3] = { 0.0 };
  const double radiusDifference = radius - currentRadius;
  this->FindLinearCoordinateByDistance(middlePoint, p1, p1New, radiusDifference);
  this->FindLinearCoordinateByDistance(middlePoint, p2, p2New, radiusDifference);
  
  if ((n % 2) == 0)
  {
    this->SetNthControlPointPositionWorld(n, p1New);
    this->SetNthControlPointPositionWorld(n + 1, p2New);
  }
  else
  {
    this->SetNthControlPointPositionWorld(n, p2New);
    this->SetNthControlPointPositionWorld(n - 1, p1New);
  }
}
