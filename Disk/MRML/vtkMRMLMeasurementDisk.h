#ifndef VTKMRMLMEASUREMENTDISK_H
#define VTKMRMLMEASUREMENTDISK_H

// MRML includes
#include "vtkMRMLMeasurement.h"

// Markups includes
#include "vtkSlicerDiskModuleMRMLExport.h"

class VTK_SLICER_DISK_MODULE_MRML_EXPORT vtkMRMLMeasurementDisk : public vtkMRMLMeasurement
{
public:
    static vtkMRMLMeasurementDisk *New();
    vtkTypeMacro(vtkMRMLMeasurementDisk, vtkMRMLMeasurement);
    void PrintSelf(ostream& os, vtkIndent indent) override;
    
    // Needed since 3fd4633168
    VTK_NEWINSTANCE
    virtual vtkMRMLMeasurementDisk* CreateInstance() const override
    { return vtkMRMLMeasurementDisk::New(); }
    void Compute() override;
    
protected:
    vtkMRMLMeasurementDisk();
    ~vtkMRMLMeasurementDisk() override;
    vtkMRMLMeasurementDisk(const vtkMRMLMeasurementDisk&);
    void operator=(const vtkMRMLMeasurementDisk&);
    
};

#endif // VTKMRMLMEASUREMENTDISK_H
