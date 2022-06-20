#ifndef VTKMRMLMEASUREMENTSPHERE_H
#define VTKMRMLMEASUREMENTSPHERE_H

// MRML includes
#include "vtkMRMLMeasurement.h"

// Markups includes
#include "vtkSlicerSphereModuleMRMLExport.h"

class VTK_SLICER_SPHERE_MODULE_MRML_EXPORT vtkMRMLMeasurementSphere : public vtkMRMLMeasurement
{
public:
    static vtkMRMLMeasurementSphere *New();
    vtkTypeMacro(vtkMRMLMeasurementSphere, vtkMRMLMeasurement);
    void PrintSelf(ostream& os, vtkIndent indent) override;
    
    // Needed since 3fd4633168
    VTK_NEWINSTANCE
    virtual vtkMRMLMeasurementSphere* CreateInstance() const override
    { return vtkMRMLMeasurementSphere::New(); }
    void Compute() override;
    
protected:
    vtkMRMLMeasurementSphere();
    ~vtkMRMLMeasurementSphere() override;
    vtkMRMLMeasurementSphere(const vtkMRMLMeasurementSphere&);
    void operator=(const vtkMRMLMeasurementSphere&);
};

#endif // VTKMRMLMEASUREMENTSPHERE_H
