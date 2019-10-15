#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>

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

#include "cJSON.h"

void Usage() {

	printf(
		"Usage: VTKSimplification [-help]\n"
		"    [-in vtk file name(full path)]\n"
		"    [-outdir  output directory(full path)]\n"
		"    [-layer number of layers you need to simplificate]\n"
		"    [-maxsize max memory size(MB) divided into pieces]\n"
		"\n"
	);

	std::cout << "example : " << std::endl;
	std::cout << "-in F:\\VTKSimplification\\data\\shanyin1.vtk -outdir F:\\VTKSimplification\\data\\test\\ -layer 5 -maxsize 5.0" << std::endl;
}

std::string GetRealPath(std::string path) {
	
	int strSize = path.size();
	std::string dest;
	dest.resize(strSize);
	int dest_id = 0;
	for (int i = 0 ; i < strSize;++i)
	{
		
		if (path[i] == '\\' || path[i] == '/')
		{
			if (i > 0 && (path[i-1] == '\\' || path[i-1] == '/'))
				continue;
		}

		if(path[i] == '\\')
			dest[dest_id++] = '/';
		else
			dest[dest_id++] = path[i];
	}

	return dest;
}

int main(int argc, char **argv) {

	if (argc > 1 && "-help" == std::string(argv[1])) {
		Usage();
		//system("pause");
		return 0;
	}

	if (argc < 8) {
		Usage();
		std::cout << "!!!!!!!!!!!!!!!!!!have no enough parameters!!!!!!!!!!!!!!!!!!" << std::endl;
		//system("pause");
		return 0;
	}

	std::string filename;
	std::string out_dir;
	int number_layer = 5;
	float splited_memeory_size = 5.0;//MB

	for (int i = 1; i < argc; ++i)
	{
		if ("-in" == std::string(argv[i]))
			filename = argv[i + 1];
		if ("-outdir" == std::string(argv[i]))
			out_dir = argv[i + 1];
		if ("-layer" == std::string(argv[i]))
			number_layer =atoi(argv[i + 1]);
		if ("-maxsize" == std::string(argv[i]))
			splited_memeory_size = atof(argv[i + 1]);
	}
	if (filename.empty() || out_dir.empty()) {
		std::cout << "filename or out put dir is empty" << std::endl;
		//system("pause");
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////
	vtkSmartPointer<vtkPolyDataReader> vtkreader = vtkSmartPointer<vtkPolyDataReader>::New();
	vtkreader->SetFileName(filename.c_str());
	vtkreader->Update();
	vtkSmartPointer<vtkPolyData> polyData = vtkreader->GetOutput();
	if (!polyData) {
		std::cout << "failed to read file : " << filename<< std::endl;
		//system("pause");
		return 0;
	}
	//vtkSmartPointer<vtkPoints> vertices = polyData->GetPoints();
	//vtkSmartPointer<vtkDataArray> verticesArray = vertices->GetData();
	vtkSmartPointer<vtkPointData> pd = polyData->GetPointData();
	vtkSmartPointer<vtkDataArray> vc = pd->GetVectors("U");

	int nc = vc->GetNumberOfComponents();
	long numberOfVertices = vc->GetNumberOfTuples();
	float f_max = -FLT_MAX;
	float f_min = FLT_MAX;
	for (int i = 0; i < numberOfVertices; i++)
	{
		float x = vc->GetComponent(i, 0);
		float y = vc->GetComponent(i, 1);
		float z = vc->GetComponent(i, 2);
		float d = x * x + y * y + z * z;
		f_max = f_max < d ? d : f_max;
		f_min = f_min > d ? d : f_min;
	}
	f_max = std::sqrt(f_max);
	f_min = std::sqrt(f_min);
	//////////////////////////////////////////////////////////////////////////
	//创建一个空的文档（对象）
	cJSON *json = cJSON_CreateObject();
	cJSON *obj_speed = NULL;
	cJSON_AddItemToObject(json,"speed", obj_speed = cJSON_CreateObject());
	cJSON_AddItemToObject(obj_speed, "min", cJSON_CreateNumber(f_min));
	cJSON_AddItemToObject(obj_speed, "max", cJSON_CreateNumber(f_max));

	//////////////////////////////////////////////////////////////////////////
	cJSON *array = NULL;
	cJSON_AddItemToObject(json, "root", array = cJSON_CreateArray());

	bool isWirtePly = false;
	int number_blocks = int(splited_memeory_size*20000);
	for (int i = 0; i < number_layer; ++i)
	{
		cJSON *obj = NULL;
		cJSON_AddItemToArray(array, obj = cJSON_CreateObject());
		cJSON_AddItemToObject(obj, "level", cJSON_CreateNumber(i + 1));

		std::cout << "processing with layer :  " << i + 1 << " of all " << number_layer << "  layers" << std::endl;
		//double percentage = i*1.0 / (number_layer-1);
		double percentage = (i+1) * 1.0 / (number_layer + 1);
		cJSON_AddItemToObject(obj, "SSE", cJSON_CreateNumber(percentage*100));

		cJSON *array_data = NULL;
		cJSON_AddItemToObject(obj, "data", array_data = cJSON_CreateArray());
		
		vtkSmartPointer<vtkDecimatePro> decimate = vtkSmartPointer<vtkDecimatePro>::New();
		decimate->SetInputData(polyData);
		decimate->SetTargetReduction(percentage); //10% reduction (if there was 100 triangles, now there will be 90)
		decimate->Update();
		//dir name
		std::string layer_dir = out_dir + "/layer_" + std::to_string(i + 1)+"/";
		vtksys::SystemTools::MakeDirectory(layer_dir.c_str());
		//splite mesh
		//vtkSmartPointer<vtkPolyData> decimated =vtkSmartPointer<vtkPolyData>::New();
		//decimated->ShallowCopy(decimate->GetOutput());
		vtkSmartPointer<vtkPolyData> decimated = decimate->GetOutput();
		int number_facets = decimated->GetNumberOfPolys();
		if (number_facets > number_blocks) {
			int number_piece = number_facets / number_blocks + 1; 
			// Create pipeline
			vtkSmartPointer<vtkOBBDicer> dicer = vtkSmartPointer<vtkOBBDicer>::New();
			dicer->SetInputData(decimated);
			dicer->SetNumberOfPieces(number_piece);
			dicer->SetDiceModeToSpecifiedNumberOfPieces();
			dicer->Update();

			vtkSmartPointer<vtkThreshold> selector =vtkSmartPointer<vtkThreshold>::New();
			selector->SetInputArrayToProcess(0, 0, 0, 0, "vtkOBBDicer_GroupIds");
			selector->SetInputConnection(dicer->GetOutputPort());
			selector->AllScalarsOff();
			for (int j = 0; j < dicer->GetNumberOfActualPieces(); ++j)
			{
				selector->ThresholdBetween(j, j);
				selector->SetComponentModeToUseAll();
				selector->Update();
				vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
				vtkSmartPointer<vtkGeometryFilter> geometry =vtkSmartPointer<vtkGeometryFilter>::New();
				geometry->SetInputConnection(selector->GetOutputPort());
				geometry->Update();
				writer->SetInputConnection(geometry->GetOutputPort());
				//double bounds[6];
				//geometry->GetOutput()->GetBounds(bounds);

				std::string filename_save = layer_dir + "/split_file_"+std::to_string(j+1)+".vtk";
				filename_save = GetRealPath(filename_save);
				writer->SetFileName(filename_save.c_str());
				writer->Write();
				std::string url_name = "/layer_" + std::to_string(i + 1) + "/split_file_" + std::to_string(j + 1) + ".vtk";
				cJSON_AddItemToArray(array_data, cJSON_CreateString(url_name.c_str()));
				if(isWirtePly)
				{
					std::string filename_ply = layer_dir + "/split_file_" + std::to_string(j + 1) + ".ply";
					vtkSmartPointer<vtkPLYWriter> plyWriter = vtkSmartPointer<vtkPLYWriter>::New();
					plyWriter->SetFileName(filename_ply.c_str());
					plyWriter->SetInputConnection(geometry->GetOutputPort());
					plyWriter->Write();
				}
			}
		}
		
		std::string filename_save = layer_dir + "/split_file_0.vtk";
		filename_save = GetRealPath(filename_save);
		vtkSmartPointer<vtkPolyDataWriter> vtkwriter = vtkSmartPointer<vtkPolyDataWriter>::New();
		vtkwriter->SetInputData(decimated);
		vtkwriter->SetFileName(filename_save.c_str());
		vtkwriter->Write();
		if (number_facets <= number_blocks) {
			std::string url_name = "/layer_" + std::to_string(i + 1) + "/split_file_0.vtk";
			cJSON_AddItemToArray(array_data, cJSON_CreateString(url_name.c_str()));
		}

	}

	char *buf = cJSON_Print(json);
	cJSON_Delete(json);

	std::string res(buf);

	std::string foutname = out_dir + "/index.json";
	foutname = GetRealPath(foutname);
	std::ofstream fout(foutname);
	if(fout.is_open())
		fout << res;
	
	fout.close();
	//system("pause");
	return 0;
}