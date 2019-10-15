#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>

#include <vtkDecimatePro.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkPLYWriter.h>
#include <vtkPolyData.h>
#include <vtksys/SystemTools.hxx>
#include <vtkOBBDicer.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkInformationDoubleVectorKey.h>
#include <vtkExtractEdges.h>
#include <vtkLine.h>
#include <vtkPolyDataAlgorithm.h>

#include "cJSON.h"

void ExtractEdges() {

}
int ReadMesh(std::string filename) {

	//////////////////////////////////////////////////////////////////////////
	vtkSmartPointer<vtkPolyDataReader> vtkreader = vtkSmartPointer<vtkPolyDataReader>::New();
	vtkreader->SetFileName(filename.c_str());
	vtkreader->Update();
	vtkSmartPointer<vtkPolyData> polyData = vtkreader->GetOutput();
	if (!polyData) {
		std::cout << "failed to read file : " << filename << std::endl;
		return 0;
	}

	//vtkIdType num_cells =  polyData->GetNumberOfCells();
	//vtkIdType num_points = polyData->GetNumberOfPoints();
	std::cout << "begin constructing lines" << std::endl;

	clock_t s = clock();
	vtkSmartPointer<vtkExtractEdges> extractEdges =vtkSmartPointer<vtkExtractEdges>::New();
	extractEdges->SetInputConnection(vtkreader->GetOutputPort());
	extractEdges->Update();
	clock_t t = clock();
	std::cout << "finish constructing lines " <<t -s <<"ms"<< std::endl;
	vtkPolyData *linesPolyData = extractEdges->GetOutput();
	vtkCellArray* lines = linesPolyData->GetLines();
	vtkPoints*    points = linesPolyData->GetPoints();
	int numpts = points->GetNumberOfPoints();
	//////////////////////////////////////////////////////////////////////////
	//创建一个空的文档（对象）
	cJSON *json = cJSON_CreateObject();
	cJSON *obj_points = NULL;
	cJSON_AddItemToObject(json, "points", obj_points = cJSON_CreateObject());
	cJSON *points_data = NULL;
	cJSON_AddItemToObject(obj_points, "geometry", points_data = cJSON_CreateArray());
	cJSON_AddItemToObject(obj_points, "size", cJSON_CreateNumber(numpts));
	
	clock_t t1 = clock();
	for (int i = 0; i < numpts;++i)
	{
		double* pt =  linesPolyData->GetPoint(i);
		cJSON_AddItemToArray(points_data, cJSON_CreateNumber(pt[0]));
		cJSON_AddItemToArray(points_data, cJSON_CreateNumber(pt[1]));
		cJSON_AddItemToArray(points_data, cJSON_CreateNumber(pt[2]));
	}
	clock_t t2 = clock();
	std::cout << "finish reading points " <<t2-t1<< std::endl;
	//////////////////////////////////////////////////////////////////////////
	cJSON *obj_lines = NULL;
	cJSON_AddItemToObject(json, "lines", obj_lines = cJSON_CreateObject());
	cJSON *lines_data = NULL;
	cJSON_AddItemToObject(obj_lines, "geometry", lines_data = cJSON_CreateArray());
	cJSON_AddItemToObject(obj_lines, "size", cJSON_CreateNumber(lines->GetNumberOfCells()));

	vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
	linesPolyData->GetLines()->InitTraversal();
	while (linesPolyData->GetLines()->GetNextCell(idList))
	{
		//std::cout << "Line has " << idList->GetNumberOfIds() << " points." << std::endl;
		for (vtkIdType pointId = 0; pointId < idList->GetNumberOfIds(); pointId++)
		{
			cJSON_AddItemToArray(lines_data, cJSON_CreateNumber(idList->GetId(pointId)));
		}
	}
	std::cout << "finish reading lines" << std::endl;
	char *buf = cJSON_Print(json);
	cJSON_Delete(json);

	std::string res(buf);

	std::string foutname = "lines.json";
	std::ofstream fout(foutname);
	if (fout.is_open())
		fout << res;

	fout.close();
}

int main(int argc, char **argv) {

	ReadMesh("F:/VTKProgram/VTKSimplification/data/read/terrainfaceSet_0.vtk");
	return 0;
}