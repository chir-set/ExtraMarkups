#ifndef VTKMRMLMEASUREMENTRING_H
#define VTKMRMLMEASUREMENTRING_H

// MRML includes
#include "vtkMRMLMeasurement.h"

// Markups includes
#include "vtkSlicerRingModuleMRMLExport.h"

class VTK_SLICER_RING_MODULE_MRML_EXPORT vtkMRMLMeasurementRing : public vtkMRMLMeasurement
{
public:
    static vtkMRMLMeasurementRing *New();
    vtkTypeMacro(vtkMRMLMeasurementRing, vtkMRMLMeasurement);
    void PrintSelf(ostream& os, vtkIndent indent) override;
    
    // Needed since 3fd4633168
    VTK_NEWINSTANCE
    virtual vtkMRMLMeasurementRing* CreateInstance() const override
      { return vtkMRMLMeasurementRing::New(); }
    void Compute() override;

protected:
    vtkMRMLMeasurementRing();
    ~vtkMRMLMeasurementRing() override;
    vtkMRMLMeasurementRing(const vtkMRMLMeasurementRing&);
    void operator=(const vtkMRMLMeasurementRing&);

};

#endif // VTKMRMLMEASUREMENTRING_H
