#ifndef CLASSIFER_NEURALNET
#define CLASSIFER_NEURALNET

#include "mzSample.h"
#include <nnwork.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>


using namespace std;

class ClassifierNeuralNet: public Classifier
{
public:
	ClassifierNeuralNet();
	~ClassifierNeuralNet();
	void classify(PeakGroup* grp);
    void train(vector<PeakGroup*>& groups);
    void refineModel(PeakGroup* grp);
    void saveModel(string filename);
    void loadModel(string filename);
    void loadDefaultModel();
	bool hasModel();
 
	vector<float> getFeatures(Peak& p);

	//neural net specific features
	nnwork* brain;
	int hidden_layer;
	int num_outputs;
    float trainingSize;
    string defaultModel;

};
#endif
