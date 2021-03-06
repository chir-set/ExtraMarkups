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

#include "vtkMRMLMarkupsLabelNode.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>

//--------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsLabelNode);

//--------------------------------------------------------------------------------
vtkMRMLMarkupsLabelNode::vtkMRMLMarkupsLabelNode()
{
  /*
   * p1 : near the label
   * p2 : near what we want to label
   */
  this->MaximumNumberOfControlPoints = 2;
  this->RequiredNumberOfControlPoints = 2;
}

//--------------------------------------------------------------------------------
vtkMRMLMarkupsLabelNode::~vtkMRMLMarkupsLabelNode()=default;

//----------------------------------------------------------------------------
void vtkMRMLMarkupsLabelNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
