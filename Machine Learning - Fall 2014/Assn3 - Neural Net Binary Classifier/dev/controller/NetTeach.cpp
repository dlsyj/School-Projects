 #include <string>
#include <vector>
#include <list>
#include <iostream>
#include <assert.h>

#include "NetTeach.h"


NetTeach::NetTeach()
{
}

NetTeach::NetTeach(float learningRate, unsigned int numIter)
{
	_learningRate = learningRate;
	_numTotalIter = numIter;
}

NetTeach::NetTeach(const NetTeach& nt)
{
	_learningRate = nt._learningRate;
	_numTotalIter = nt._numTotalIter;
}

NetTeach& NetTeach::operator =(const NetTeach& nt)
{
	_learningRate = nt._learningRate;
	_numTotalIter = nt._numTotalIter;
	return *this;
}

std::vector<Instance> NetTeach::getExamplesFromFile(std::string& filename)
{
	//open the file
	std::ifstream filestream;
	filestream.open(filename);

	std::string line;
	if(filestream.is_open()){

		//grab the first line, it contains the attribute names
		getline(filestream, line);

		//delimiters include tab and space
		const char* delim = " \t";
		
		//placeholders for tokenizing the current line, these keep the tokenizer moving forward so we don't acknowledge the same space more than once
		size_t prev = 0;
		size_t next = 0;

		if(_attrNames.size() != 0){
			_attrNames.clear();
		}
		while((next = line.find_first_of(delim, prev)) != std::string::npos){
			
				//if the token we got is not an empty string, add it to _attrNames
				if(next - prev != 0){
					_attrNames.push_back(line.substr(prev, next - prev));
				}
				//update the placeholder
				prev = next + 1;
			}

			//reset some variables for the next loop around
			prev = 0; next = 0;

		//gonna grab these from the file. The rest of the lines will contain attibute and class values
		unsigned int numValues = 0;
		std::vector<unsigned int> values;
		unsigned int classVal;

		//examples container that we will return
		std::vector<Instance> examples;

		while(getline(filestream, line)){
			while((next = line.find_first_of(delim, prev)) != std::string::npos){
			
				//if the token we got is not an empty string, add it to values
				if(next - prev != 0){
					values.push_back(atoi(line.substr(prev, next - prev).c_str()));
				}
				//update the placeholder
				prev = next + 1;
			}

			//grab the last token and assign it to classVal
			if(prev < line.size()){
				classVal = atoi(line.substr(prev, next - prev).c_str());
			}

			numValues = values.size();

			//create the Instance object, one line equals one instance
			Instance instance(numValues, values, classVal);

			//add it to examples container
			examples.push_back(instance);

			//reset some variables for the next loop around
			prev = 0; next = 0;
			values.clear();
		}
		filestream.close();


		return examples;
	
	}else{
	
		std::cout << "unable to open file " << filename << std::endl;
		exit(1);
	
	}
}

void NetTeach::setTrainingExamples(std::vector<Instance> trainex){
	_trainex = trainex;
}

void NetTeach::setTestingExamples(std::vector<Instance> testex){
	_testex = testex;
}

float NetTeach::teach(Net& net, unsigned int iterationNum)
{
	//first the net has to build itself
	
	//in the input layer there will be an input for each attribute type
	net.buildInputLayer(_attrNames.size());

	//we have the data we need from building the input layer to build the hidden layer
	net.buildHiddenLayer();

	//the output layer will just have one output node
	net.buildOutputLayer();

	std::vector<float> results;

	for(unsigned int i = 0; i < _trainex.size(); i++){
		setInputs(net, iterationNum);
		forwardPropogate(net);
		backPropogate(net, i);

		results.push_back(net._outputLayer[0]._output);

		//std::cout << net._inputLayer << std::endl;
		//std::cout << net._hiddenLayer << std::endl;
		//std::cout << net._outputLayer << std::endl;
	}
	testAccuracyOnSet(_trainex, results);

	return 0;
}

void NetTeach::setInputs(Net& net, unsigned int iterationNum){
	for(int i = 0; i < net._inputLayer.size(); i++){
		net._inputLayer[i]._inputVal = _trainex[iterationNum]._values[i];
	}
}

void NetTeach::forwardPropogate(Net& net)
{
	//cycle through all the neurons in the network and calculate the output values for all of them.
	//we don't need to calculate ouputs for input neurons
	float fromInputNeuron_i;
	float sumFromInputs = 0;
	float outputForHidden_i;
	for(int i = 0; i < net._hiddenLayer.size(); i++){
		for(int j = 0; j < net._inputLayer.size(); j++){
			//multiply the input from that neuron by the connecting weight
			fromInputNeuron_i = net._inputLayer[j]._inputVal * net.findWeight(net.weightController.genWeightName(j, 'i', i, 'h'))._val;
			sumFromInputs += fromInputNeuron_i;
		}
		outputForHidden_i = net.neuronController.sigmoid(sumFromInputs);
		net._hiddenLayer[i]._output = outputForHidden_i;
	}

	float fromHiddenNeuron_i;
	float sumFromHiddens = 0;
	float finalOutput;
	for(int i = 0; i < net._hiddenLayer.size(); i++){
		fromHiddenNeuron_i = net._hiddenLayer[i]._output * net.findWeight(net.weightController.genWeightName(i, 'h', 1, 'o'))._val;
		sumFromHiddens += fromHiddenNeuron_i;
	}
	finalOutput = net.neuronController.sigmoid(sumFromHiddens);
	net._outputLayer[0]._output = finalOutput;
}

void NetTeach::backPropogate(Net& net, unsigned int currentTrainingInstance)
{
	int classVal = _trainex[currentTrainingInstance]._class;

	//generate delta for the output node
	float errorOutputNeuron = net._outputLayer[0]._output - classVal;
	float deltaOutputNeuron = errorOutputNeuron * net.neuronController.partialDeriv(net._outputLayer[0]._sumOfIncomingtWeights);

	//generate delta for all hidden nodes
	for(int i =  0; i < net._hiddenLayer.size(); i++){
		//delta for hidden neurons is deriv(sum incoming weights) * sumOutgoingWeights * deltaOutputNeuron
		net._hiddenLayer[i]._delta = net.neuronController.partialDeriv(net._hiddenLayer[i]._sumOfIncomingtWeights) * net._weights[net._hiddenLayer[i]._outgoingWeights[0]]._val * deltaOutputNeuron;
	}

	std::string weightname;
	//calculate all the gradients of all the weights
	for(int i = 0; i < net._weights.size(); i++){
		weightname = net._weights[i]._name;

		//if current weight connects an input node and a hidden node
		if(weightname[2] == 'i'){
			int currentInputNodeIndex = net.weightController.getWeightParentNeuronIndex(weightname);
			net._weights[i]._gradient = net._inputLayer[currentInputNodeIndex]._inputVal * deltaOutputNeuron;
		}else if(weightname[2] == 'h'){
			int currentHiddenNodeIndex = net.weightController.getWeightParentNeuronIndex(weightname);
			net._weights[i]._gradient = net._hiddenLayer[currentHiddenNodeIndex]._output * deltaOutputNeuron;
		}
	}

	//calculate all the weight updates
	for(int i = 0; i < net._weights.size(); i++){
		float newWeight = ((-1) * _learningRate * net._weights[i]._gradient) + (net._momentum * net._weights[i]._previousVal);
		net._weights[i]._previousVal = net._weights[i]._val;
		net._weights[i]._val = newWeight;
	}
}

float NetTeach::testAccuracyOnSet(std::vector<Instance> examples, std::vector<float>& results)
{
	int numRight = 0;
	int numWrong = 0;
	for(int i = 0; i < examples.size(); i++){
		std::cout << "trex: " << examples[i]._class << " res: " << (int) (results[i] + 0.5) << std::endl;
		if(examples[i]._class == (int) (results[i] + 0.5)){
			numRight++;
		}else{
			numWrong++;
		}
	}
	float accuracy = (numRight) / (float)examples.size();
	std:: cout << "accuracy: " << std::setprecision(3) << accuracy * 100 << "%" << std::endl;
	return 0;
}
